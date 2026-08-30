using System;
using System.IO;

namespace WildStar.Texture;

internal static class TexDecoder
{
    private sealed class Huff
    {
        private readonly int[] _minCode = new int[17];
        private readonly int[] _maxCode = new int[18];
        private readonly int[] _valPtr = new int[17];
        private readonly byte[] _values;

        public Huff(byte[] bits, byte[] values)
        {
            _values = values;
            int code = 0, k = 0;

            for (int len = 1; len <= 16; len++)
            {
                _valPtr[len] = k;
                _minCode[len] = code;
                code += bits[len - 1];
                k += bits[len - 1];
                _maxCode[len] = bits[len - 1] == 0 ? -1 : code - 1;
                code <<= 1;
            }

            _maxCode[17] = int.MaxValue;
        }

        public int Decode(BitReader r)
        {
            int code = r.Bit();

            for (int len = 1; len <= 16; len++)
            {
                if (_maxCode[len] >= 0 && code <= _maxCode[len])
                {
                    return _values[_valPtr[len] + code - _minCode[len]];
                }

                code = (code << 1) | r.Bit();
            }

            throw new EndOfStreamException("bad Huffman code");
        }
    }

    private sealed class BitReader
    {
        private readonly byte[] _data;
        private readonly int _end;
        private int _pos;
        private int _bit;

        public BitReader(byte[] data, int start, int length)
        {
            _data = data;
            _pos = start;
            _end = start + length;
            _bit = 0;
        }

        public int Bit()
        {
            if (_pos >= _end)
            {
                return 0;
            }

            int v = (_data[_pos] >> (7 - _bit)) & 1;
            if (++_bit == 8)
            {
                _bit = 0;
                _pos++;
            }

            return v;
        }

        public int Bits(int n)
        {
            int v = 0;
            for (int i = 0; i < n; i++)
            {
                v = (v << 1) | Bit();
            }

            return v;
        }
    }

    private static int Extend(int v, int n) => n == 0 ? 0 : (v < (1 << (n - 1)) ? v + 1 - (1 << n) : v);

    private static float[] ScaleQuant(int[] baseTable, int quality)
    {
        float scale = (200.0f - 2.0f * quality) * 0.01f;
        var q = new float[64];

        for (int i = 0; i < 64; i++)
        {
            q[i] = Math.Clamp(scale * baseTable[i], 1.0f, 255.0f);
        }

        return q;
    }

    private static void Idct(float[] block, float[] outBlock)
    {
        Span<float> tmp = stackalloc float[64];

        for (int u = 0; u < 8; u++)
        {
            for (int x = 0; x < 8; x++)
            {
                float s = 0.0f;
                for (int v = 0; v < 8; v++)
                {
                    float cv = v == 0 ? 0.70710678f : 1.0f;
                    s += cv * block[v * 8 + u] * CosTable[v * 8 + x];
                }

                tmp[x * 8 + u] = s * 0.5f;
            }
        }

        for (int y = 0; y < 8; y++)
        {
            for (int x = 0; x < 8; x++)
            {
                float s = 0.0f;
                for (int u = 0; u < 8; u++)
                {
                    float cu = u == 0 ? 0.70710678f : 1.0f;
                    s += cu * tmp[y * 8 + u] * CosTable[u * 8 + x];
                }

                outBlock[y * 8 + x] = s * 0.5f;
            }
        }
    }

    private static readonly float[] CosTable = BuildCos();

    private static float[] BuildCos()
    {
        var t = new float[64];
        for (int u = 0; u < 8; u++)
        {
            for (int x = 0; x < 8; x++)
            {
                t[u * 8 + x] = MathF.Cos((2 * x + 1) * u * MathF.PI / 16.0f);
            }
        }

        return t;
    }

    public static byte[] DecodeLevel(byte[] data, int offset, int length, int width, int height,
                                     int pixelMode, int[] quality, bool[] isConstant,
                                     int[] constantValue)
    {

        bool yCoCg = pixelMode != 1;

        int[][] quantBase = yCoCg
            ? new[] { TexTables.LumaQuant, TexTables.ChromaQuant, TexTables.ChromaQuant, TexTables.LumaQuant }
            : new[] { TexTables.LumaQuant, TexTables.LumaQuant, TexTables.LumaQuant, TexTables.LumaQuant };

        bool[] chromaHuff = yCoCg
            ? new[] { false, true, true, false }
            : new[] { false, false, false, false };

        var quant = new float[4][];
        for (int c = 0; c < 4; c++)
        {
            quant[c] = ScaleQuant(quantBase[c], quality[c]);
        }

        var lumaDc = new Huff(TexTables.LumaDcBits, TexTables.LumaDcValues);
        var lumaAc = new Huff(TexTables.LumaAcBits, TexTables.LumaAcValues);
        var chromaDc = new Huff(TexTables.ChromaDcBits, TexTables.ChromaDcValues);
        var chromaAc = new Huff(TexTables.ChromaAcBits, TexTables.ChromaAcValues);

        var reader = new BitReader(data, offset, length);
        var pred = new int[4];

        int mcuSize = pixelMode == 0 ? 16 : 8;
        int mcuX = (width + mcuSize - 1) / mcuSize;
        int mcuY = (height + mcuSize - 1) / mcuSize;

        bool modeZero = pixelMode == 0;

        int[] compBlocks = modeZero ? new[] { 4, 1, 1, 4 } : new[] { 1, 1, 1, 1 };

        for (int c = 0; c < 4; c++)
        {
            if (isConstant[c]) compBlocks[c] = 0;
        }
        int[] compBw = modeZero ? new[] { 2, 1, 1, 2 } : new[] { 1, 1, 1, 1 };
        int[] compBh = modeZero ? new[] { 2, 1, 1, 2 } : new[] { 1, 1, 1, 1 };

        int cw = modeZero ? (width + 1) / 2 : width;
        int chh = modeZero ? (height + 1) / 2 : height;

        var planeY = new float[width * height];
        var planeA = new float[width * height];
        var planeCo = new float[cw * chh];
        var planeCg = new float[cw * chh];

        for (int c = 0; c < 4; c++)
        {
            if (!isConstant[c]) continue;

            float v = constantValue[c];
            float[] target = c switch
            {
                0 => planeY,
                3 => planeA,
                1 => planeCo,
                _ => planeCg,
            };

            float fill = (c == 1 || c == 2) && yCoCg ? v - 128.0f : v;
            Array.Fill(target, fill);
        }

        var coef = new float[64];
        var pixels = new float[64];

        try
        {
        for (int my = 0; my < mcuY; my++)
        {
            for (int mx = 0; mx < mcuX; mx++)
            {
                for (int comp = 0; comp < 4; comp++)
                {
                    int blocks = compBlocks[comp];
                    int bw = compBw[comp], bh = compBh[comp];

                    for (int b = 0; b < blocks; b++)
                    {
                        Array.Clear(coef);

                        Huff dcTable = chromaHuff[comp] ? chromaDc : lumaDc;
                        Huff acTable = chromaHuff[comp] ? chromaAc : lumaAc;

                        int t = dcTable.Decode(reader);
                        pred[comp] += Extend(reader.Bits(t), t);
                        coef[0] = pred[comp] * quant[comp][0];

                        for (int k = 1; k < 64;)
                        {
                            int rs = acTable.Decode(reader);
                            int run = rs >> 4, size = rs & 15;

                            if (size == 0)
                            {
                                if (run != 15) break;
                                k += 16;
                                continue;
                            }

                            k += run;
                            if (k > 63) break;

                            int z = TexTables.Zigzag[k];
                            coef[z] = Extend(reader.Bits(size), size) * quant[comp][z];
                            k++;
                        }

                        Idct(coef, pixels);

                        int bx = (b % bw) * 8;
                        int by = (b / bw) * 8;

                        int sx = mcuSize / (bw * 8);
                        int sy = mcuSize / (bh * 8);

                        for (int y = 0; y < 8; y++)
                        {
                            for (int x = 0; x < 8; x++)
                            {
                                float v = pixels[y * 8 + x];
                                int lx = bx + x, ly = by + y;

                                if (comp == 0 || comp == 3)
                                {

                                    int px = mx * mcuSize + lx * sx;
                                    int py = my * mcuSize + ly * sy;
                                    float lvl = Math.Clamp(v + 128.0f, 0.0f, 255.0f);

                                    for (int ry = 0; ry < sy; ry++)
                                    for (int rx = 0; rx < sx; rx++)
                                    {
                                        int tx = px + rx, ty = py + ry;
                                        if (tx >= width || ty >= height) continue;
                                        if (comp == 0) planeY[ty * width + tx] = lvl;
                                        else planeA[ty * width + tx] = lvl;
                                    }
                                }
                                else
                                {

                                    int planeW = cw;
                                    int stepX = modeZero ? 1 : sx;
                                    int stepY = modeZero ? 1 : sy;
                                    int px = mx * (modeZero ? 8 : mcuSize) + lx * stepX;
                                    int py = my * (modeZero ? 8 : mcuSize) + ly * stepY;
                                    float cval = modeZero
                                        ? v
                                        : yCoCg
                                            ? Math.Clamp(v, -256.0f, 255.0f)
                                            : Math.Clamp(v + 128.0f, 0.0f, 255.0f) - 128.0f;

                                    for (int ry = 0; ry < stepY; ry++)
                                    for (int rx = 0; rx < stepX; rx++)
                                    {
                                        int tx = px + rx, ty = py + ry;
                                        if (tx >= planeW || ty >= chh) continue;
                                        if (comp == 1) planeCo[ty * planeW + tx] = cval;
                                        else planeCg[ty * planeW + tx] = cval;
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }

        }
        catch (EndOfStreamException)
        {

        }

        var outPixels = new byte[width * height * 4];

        for (int y = 0; y < height; y++)
        {
            int cy = modeZero ? y >> 1 : y;
            for (int x = 0; x < width; x++)
            {
                int ci = cy * cw + (modeZero ? x >> 1 : x);
                float Y = planeY[y * width + x];
                float co = planeCo[ci];
                float cg = planeCg[ci];

                int o = (y * width + x) * 4;

                if (yCoCg)
                {
                    float t = Y - MathF.Floor(cg * 0.5f);
                    float g = t + cg;
                    float bl = t - MathF.Floor(co * 0.5f);
                    float r = bl + co;

                    outPixels[o] = (byte)Math.Clamp(r, 0.0f, 255.0f);
                    outPixels[o + 1] = (byte)Math.Clamp(g, 0.0f, 255.0f);
                    outPixels[o + 2] = (byte)Math.Clamp(bl, 0.0f, 255.0f);
                    outPixels[o + 3] = (byte)Math.Clamp(planeA[y * width + x], 0.0f, 255.0f);
                }
                else
                {

                    outPixels[o] = (byte)Math.Clamp(planeA[y * width + x], 0.0f, 255.0f);
                    outPixels[o + 1] = (byte)Math.Clamp(co + 128.0f, 0.0f, 255.0f);
                    outPixels[o + 2] = (byte)Math.Clamp(cg + 128.0f, 0.0f, 255.0f);
                    outPixels[o + 3] = (byte)Math.Clamp(Y, 0.0f, 255.0f);
                }
            }
        }

        return outPixels;
    }
}
