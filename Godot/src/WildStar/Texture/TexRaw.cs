using System;
using System.Buffers.Binary;

namespace WildStar.Texture;

public static class TexRaw
{
    public static bool Supports(int formatIndex) => formatIndex switch
    {
        0 or 1 or 2 or 3 or 4 or 5 or 6 or 13 or 14 or 15 => true,
        _ => false,
    };

    public static byte[] DecodeLevel(byte[] source, int offset, int size, int width, int height,
                                     int formatIndex)
    {
        var rgba = new byte[width * height * 4];

        switch (formatIndex)
        {
            case 0:
                Linear32(source, offset, size, width, height, rgba, true);
                break;
            case 1:
                Linear32(source, offset, size, width, height, rgba, false);
                break;
            case 2:
                Linear16(source, offset, size, width, height, rgba, Rgb565);
                break;
            case 3:
                Linear16(source, offset, size, width, height, rgba, Argb1555);
                break;
            case 4:
                Linear16(source, offset, size, width, height, rgba, Xrgb1555);
                break;
            case 5:
                Linear16(source, offset, size, width, height, rgba, Argb4444);
                break;
            case 6:
                Luminance8(source, offset, size, width, height, rgba);
                break;
            case 13:
                Blocks(source, offset, size, width, height, rgba, 8, DecodeBc1Block);
                break;
            case 14:
                Blocks(source, offset, size, width, height, rgba, 16, DecodeBc2Block);
                break;
            case 15:
                Blocks(source, offset, size, width, height, rgba, 16, DecodeBc3Block);
                break;
            default:
                throw new NotSupportedException("texture format index " + formatIndex);
        }

        return rgba;
    }

    private static int Stride(int bytesPerPixel, int width) => (bytesPerPixel * width + 3) & ~3;

    private static void Linear32(byte[] src, int offset, int size, int width, int height,
                                 byte[] rgba, bool useAlpha)
    {
        int stride = Stride(4, width);
        for (int y = 0; y < height; y++)
        {
            int row = offset + y * stride;
            for (int x = 0; x < width; x++)
            {
                int s = row + x * 4;
                int d = (y * width + x) * 4;
                if (s + 3 >= src.Length) return;

                rgba[d] = src[s + 2];
                rgba[d + 1] = src[s + 1];
                rgba[d + 2] = src[s];
                rgba[d + 3] = useAlpha ? src[s + 3] : (byte)255;
            }
        }
    }

    private static void Linear16(byte[] src, int offset, int size, int width, int height,
                                 byte[] rgba, Func<ushort, (byte, byte, byte, byte)> decode)
    {
        int stride = Stride(2, width);
        for (int y = 0; y < height; y++)
        {
            int row = offset + y * stride;
            for (int x = 0; x < width; x++)
            {
                int s = row + x * 2;
                int d = (y * width + x) * 4;
                if (s + 1 >= src.Length) return;

                (byte r, byte g, byte b, byte a) = decode(
                    BinaryPrimitives.ReadUInt16LittleEndian(src.AsSpan(s)));

                rgba[d] = r;
                rgba[d + 1] = g;
                rgba[d + 2] = b;
                rgba[d + 3] = a;
            }
        }
    }

    private static void Luminance8(byte[] src, int offset, int size, int width, int height,
                                   byte[] rgba)
    {
        int stride = Stride(1, width);
        for (int y = 0; y < height; y++)
        {
            int row = offset + y * stride;
            for (int x = 0; x < width; x++)
            {
                int s = row + x;
                int d = (y * width + x) * 4;
                if (s >= src.Length) return;

                rgba[d] = rgba[d + 1] = rgba[d + 2] = src[s];
                rgba[d + 3] = 255;
            }
        }
    }

    private static (byte, byte, byte, byte) Rgb565(ushort c) =>
        (Expand5((c >> 11) & 0x1F), Expand6((c >> 5) & 0x3F), Expand5(c & 0x1F), (byte)255);

    private static (byte, byte, byte, byte) Argb1555(ushort c) =>
        (Expand5((c >> 10) & 0x1F), Expand5((c >> 5) & 0x1F), Expand5(c & 0x1F),
         (c & 0x8000) != 0 ? (byte)255 : (byte)0);

    private static (byte, byte, byte, byte) Xrgb1555(ushort c) =>
        (Expand5((c >> 10) & 0x1F), Expand5((c >> 5) & 0x1F), Expand5(c & 0x1F), (byte)255);

    private static (byte, byte, byte, byte) Argb4444(ushort c) =>
        (Expand4((c >> 8) & 0xF), Expand4((c >> 4) & 0xF), Expand4(c & 0xF),
         Expand4((c >> 12) & 0xF));

    private static byte Expand4(int v) => (byte)(v * 17);

    private static byte Expand5(int v) => (byte)((v << 3) | (v >> 2));

    private static byte Expand6(int v) => (byte)((v << 2) | (v >> 4));

    private delegate void BlockDecoder(byte[] src, int at, Span<byte> block);

    private static void Blocks(byte[] src, int offset, int size, int width, int height,
                               byte[] rgba, int blockBytes, BlockDecoder decoder)
    {
        int blocksW = (width + 3) >> 2;
        int blocksH = (height + 3) >> 2;
        int stride = (blockBytes * blocksW + 3) & ~3;

        Span<byte> block = stackalloc byte[64];

        for (int by = 0; by < blocksH; by++)
        {
            for (int bx = 0; bx < blocksW; bx++)
            {
                int at = offset + by * stride + bx * blockBytes;
                if (at + blockBytes > src.Length) return;

                decoder(src, at, block);

                for (int py = 0; py < 4; py++)
                {
                    int y = by * 4 + py;
                    if (y >= height) break;

                    for (int px = 0; px < 4; px++)
                    {
                        int x = bx * 4 + px;
                        if (x >= width) break;

                        int s = (py * 4 + px) * 4;
                        int d = (y * width + x) * 4;
                        rgba[d] = block[s];
                        rgba[d + 1] = block[s + 1];
                        rgba[d + 2] = block[s + 2];
                        rgba[d + 3] = block[s + 3];
                    }
                }
            }
        }
    }

    private static void ColourBlock(byte[] src, int at, Span<byte> block, bool punchThrough)
    {
        ushort c0 = BinaryPrimitives.ReadUInt16LittleEndian(src.AsSpan(at));
        ushort c1 = BinaryPrimitives.ReadUInt16LittleEndian(src.AsSpan(at + 2));
        uint bits = BinaryPrimitives.ReadUInt32LittleEndian(src.AsSpan(at + 4));

        Span<byte> r = stackalloc byte[4];
        Span<byte> g = stackalloc byte[4];
        Span<byte> b = stackalloc byte[4];
        Span<byte> a = stackalloc byte[4];

        (r[0], g[0], b[0], _) = Rgb565(c0);
        (r[1], g[1], b[1], _) = Rgb565(c1);
        a[0] = a[1] = a[2] = a[3] = 255;

        if (c0 > c1 || !punchThrough)
        {
            r[2] = (byte)((2 * r[0] + r[1]) / 3);
            g[2] = (byte)((2 * g[0] + g[1]) / 3);
            b[2] = (byte)((2 * b[0] + b[1]) / 3);
            r[3] = (byte)((r[0] + 2 * r[1]) / 3);
            g[3] = (byte)((g[0] + 2 * g[1]) / 3);
            b[3] = (byte)((b[0] + 2 * b[1]) / 3);
        }
        else
        {
            r[2] = (byte)((r[0] + r[1]) / 2);
            g[2] = (byte)((g[0] + g[1]) / 2);
            b[2] = (byte)((b[0] + b[1]) / 2);
            r[3] = g[3] = b[3] = 0;
            a[3] = 0;
        }

        for (int i = 0; i < 16; i++)
        {
            int code = (int)((bits >> (i * 2)) & 3);
            int d = i * 4;
            block[d] = r[code];
            block[d + 1] = g[code];
            block[d + 2] = b[code];
            block[d + 3] = a[code];
        }
    }

    private static void DecodeBc1Block(byte[] src, int at, Span<byte> block) =>
        ColourBlock(src, at, block, true);

    private static void DecodeBc2Block(byte[] src, int at, Span<byte> block)
    {
        ColourBlock(src, at + 8, block, false);

        for (int i = 0; i < 16; i++)
        {
            int nibble = src[at + (i >> 1)];
            int value = (i & 1) == 0 ? nibble & 0xF : nibble >> 4;
            block[i * 4 + 3] = Expand4(value);
        }
    }

    private static void DecodeBc3Block(byte[] src, int at, Span<byte> block)
    {
        ColourBlock(src, at + 8, block, false);

        Span<byte> alpha = stackalloc byte[8];
        alpha[0] = src[at];
        alpha[1] = src[at + 1];

        if (alpha[0] > alpha[1])
        {
            for (int i = 1; i < 7; i++)
            {
                alpha[i + 1] = (byte)(((7 - i) * alpha[0] + i * alpha[1]) / 7);
            }
        }
        else
        {
            for (int i = 1; i < 5; i++)
            {
                alpha[i + 1] = (byte)(((5 - i) * alpha[0] + i * alpha[1]) / 5);
            }

            alpha[6] = 0;
            alpha[7] = 255;
        }

        ulong codes = 0;
        for (int i = 0; i < 6; i++)
        {
            codes |= (ulong)src[at + 2 + i] << (i * 8);
        }

        for (int i = 0; i < 16; i++)
        {
            block[i * 4 + 3] = alpha[(int)((codes >> (i * 3)) & 7)];
        }
    }
}
