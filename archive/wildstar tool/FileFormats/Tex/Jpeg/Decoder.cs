using System;
using System.Collections.Generic;

namespace ProjectWS.FileFormats.Tex.Jpeg
{
    public class Decoder
    {
        static bool initialized;

        const uint MaxACDCLength = 16;
        readonly byte[] dcLuminanceLengths = { 0, 1, 5, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0 };
        readonly byte[] acLuminanceLengths = { 0, 2, 1, 3, 3, 2, 4, 3, 5, 5, 4, 4, 0, 0, 1, 125 };
        readonly byte[] dcChrominanceLenghts = { 0, 3, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0 };
        readonly byte[] acChrominanceLenghts = { 0, 2, 1, 2, 4, 4, 3, 4, 7, 5, 4, 4, 0, 1, 2, 119 };
        readonly byte[] dcLuminanceValues = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11 };
        readonly byte[] acLuminanceValues = { 0x01, 0x02, 0x03, 0x00, 0x04, 0x11, 0x05, 0x12, 0x21, 0x31, 0x41, 0x06, 0x13, 0x51,
                                               0x61, 0x07, 0x22, 0x71, 0x14, 0x32, 0x81, 0x91, 0xA1, 0x08, 0x23, 0x42, 0xB1, 0xC1,
                                               0x15, 0x52, 0xD1, 0xf0, 0x24, 0x33, 0x62, 0x72, 0x82, 0x09, 0x0A, 0x16, 0x17, 0x18,
                                               0x19, 0x1A, 0x25, 0x26, 0x27, 0x28, 0x29, 0x2A, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39,
                                               0x3A, 0x43, 0x44, 0x45, 0x46, 0x47, 0x48, 0x49, 0x4A, 0x53, 0x54, 0x55, 0x56, 0x57,
                                               0x58, 0x59, 0x5A, 0x63, 0x64, 0x65, 0x66, 0x67, 0x68, 0x69, 0x6A, 0x73, 0x74, 0x75,
                                               0x76, 0x77, 0x78, 0x79, 0x7A, 0x83, 0x84, 0x85, 0x86, 0x87, 0x88, 0x89, 0x8A, 0x92,
                                               0x93, 0x94, 0x95, 0x96, 0x97, 0x98, 0x99, 0x9A, 0xA2, 0xA3, 0xA4, 0xa5, 0xA6, 0xA7,
                                               0xA8, 0xA9, 0xAA, 0xB2, 0xB3, 0xB4, 0xB5, 0xB6, 0xB7, 0xB8, 0xB9, 0xBA, 0xC2, 0xC3,
                                               0xC4, 0xC5, 0xC6, 0xC7, 0xC8, 0xC9, 0xCA, 0xD2, 0xD3, 0xD4, 0xD5, 0xD6, 0xD7, 0xD8,
                                               0xD9, 0xDA, 0xE1, 0xE2, 0xE3, 0xE4, 0xE5, 0xE6, 0xE7, 0xE8, 0xE9, 0xEa, 0xF1, 0xF2,
                                               0xF3, 0xF4, 0xF5, 0xF6, 0xF7, 0xF8, 0xF9, 0xFa };
        readonly byte[] dcChrominanceValues = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11 };
        readonly byte[] acChrominanceValues = { 0x00, 0x01, 0x02, 0x03, 0x11, 0x04, 0x05, 0x21, 0x31, 0x06, 0x12, 0x41, 0x51, 0x07,
                                                 0x61, 0x71, 0x13, 0x22, 0x32, 0x81, 0x08, 0x14, 0x42, 0x91, 0xA1, 0xB1, 0xC1, 0x09,
                                                 0x23, 0x33, 0x52, 0xF0, 0x15, 0x62, 0x72, 0xD1, 0x0A, 0x16, 0x24, 0x34, 0xE1, 0x25,
                                                 0xF1, 0x17, 0x18, 0x19, 0x1A, 0x26, 0x27, 0x28, 0x29, 0x2A, 0x35, 0x36, 0x37, 0x38,
                                                 0x39, 0x3A, 0x43, 0x44, 0x45, 0x46, 0x47, 0x48, 0x49, 0x4A, 0x53, 0x54, 0x55, 0x56,
                                                 0x57, 0x58, 0x59, 0x5a, 0x63, 0x64, 0x65, 0x66, 0x67, 0x68, 0x69, 0x6A, 0x73, 0x74,
                                                 0x75, 0x76, 0x77, 0x78, 0x79, 0x7a, 0x82, 0x83, 0x84, 0x85, 0x86, 0x87, 0x88, 0x89,
                                                 0x8A, 0x92, 0x93, 0x94, 0x95, 0x96, 0x97, 0x98, 0x99, 0x9A, 0xa2, 0xA3, 0xA4, 0xA5,
                                                 0xA6, 0xA7, 0xA8, 0xA9, 0xAA, 0xB2, 0xB3, 0xB4, 0xB5, 0xB6, 0xB7, 0xB8, 0xB9, 0xBA,
                                                 0xC2, 0xC3, 0xC4, 0xC5, 0xC6, 0xC7, 0xC8, 0xC9, 0xCA, 0xD2, 0xD3, 0xD4, 0xD5, 0xD6,
                                                 0xD7, 0xD8, 0xD9, 0xDA, 0xE2, 0xE3, 0xE4, 0xE5, 0xE6, 0xE7, 0xE8, 0xE9, 0xEA, 0xF2,
                                                 0xF3, 0xF4, 0xF5, 0xF6, 0xF7, 0xF8, 0xF9, 0xFA };
        readonly byte[] zigzagMapping = { 0, 1, 5, 6, 14, 15, 27, 28, 2, 4, 7, 13, 16, 26, 29, 42, 3, 8, 12, 17, 25, 30, 41, 43,
                                            9, 11, 18, 24, 31, 40, 44, 53, 10, 19, 23, 32, 39, 45, 52, 54, 20, 22, 33, 38, 46, 51, 55,
                                            60, 21, 34, 37, 47, 50, 56, 59, 61, 35, 36, 48, 49, 57, 58, 62, 63 };
        readonly byte[] lumQuantizationTemplate = { 16, 11, 10, 16, 24, 40, 51, 61, 12, 12, 14, 19, 26, 58, 60, 55,
                                                    14, 13, 16, 24, 40, 57, 69, 56, 14, 17, 22, 29, 51, 87, 80, 62,
                                                    18, 22, 37, 56, 68, 109, 103, 77, 24, 35, 55, 64, 81, 104, 113, 92,
                                                    49, 64, 78, 87, 103, 121, 120, 101, 72, 92, 95, 98, 112, 100, 103, 99 };
        readonly byte[] chromaQuantizationTemplate = { 17, 18, 24, 47, 99, 99, 99, 99, 18, 21, 26, 66, 99, 99, 99, 99,
                                                       24, 26, 56, 99, 99, 99, 99, 99, 47, 66, 99, 99, 99, 99, 99, 99,
                                                       99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99,
                                                       99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99 };

        static float[][] luminanceQuantizationTables = new float[100][];
        static float[][] chrominanceQuantizationTables = new float[100][];

        static Dictionary<long, byte> dcLuminanceTable;
        static Dictionary<long, byte> acLuminanceTable;
        static Dictionary<long, byte> dcChrominanceTable;
        static Dictionary<long, byte> acChrominanceTable;

        /// <summary>
        /// Decompress a jpeg compressed mip level to RGBA32
        /// </summary>
        /// <param name="header"></param>
        /// <param name="imageData"></param>
        /// <returns></returns>
        public byte[] DecompressMip(Tex.Header header, int mipLevel, byte[] imageData)
        {
            Initialize();

            float[][] luminanceQuantizationTable = new float[4][];
            float[][] chrominanceQuantizationTable = new float[4][];

            byte[] qualityLevels = new byte[4]
            {
                (byte)header.layerInfos[0].quality,
                (byte)header.layerInfos[1].quality,
                (byte)header.layerInfos[2].quality,
                (byte)header.layerInfos[3].quality
            };

            for (var i = 0u; i < 4; i++)
            {
                if (qualityLevels[i] > 100)
                {
                    Console.WriteLine("Quality level > 100 ?");
                    return null;
                }

                luminanceQuantizationTable[i] = luminanceQuantizationTables[qualityLevels[i]];
                chrominanceQuantizationTable[i] = chrominanceQuantizationTables[qualityLevels[i]];
            }

            BitStream data = new BitStream(imageData);

            int width = (int)(header.width / Math.Pow(2, mipLevel));
            int height = (int)(header.height / Math.Pow(2, mipLevel));

            return DecompressMipInternal(width, height, data, header.compressionFormat, dcLuminanceTable, acLuminanceTable, dcChrominanceTable, acChrominanceTable, header.layerInfos, luminanceQuantizationTable, chrominanceQuantizationTable);
        }

        void Initialize()
        {
            if (initialized) return;

            //IDCT.Initialize();

            dcLuminanceTable = GenerateHuffmanTable(dcLuminanceLengths, MaxACDCLength, dcLuminanceValues);
            acLuminanceTable = GenerateHuffmanTable(acLuminanceLengths, MaxACDCLength, acLuminanceValues);
            dcChrominanceTable = GenerateHuffmanTable(dcChrominanceLenghts, MaxACDCLength, dcChrominanceValues);
            acChrominanceTable = GenerateHuffmanTable(acChrominanceLenghts, MaxACDCLength, acChrominanceValues);

            for (byte i = 0; i < 100; i++)
            {
                luminanceQuantizationTables[i] = GenerateQuantizationTable(i, lumQuantizationTemplate);
                chrominanceQuantizationTables[i] = GenerateQuantizationTable(i, chromaQuantizationTemplate);
            }

            initialized = true;
        }

        Dictionary<long, byte> GenerateHuffmanTable(byte[] lengths, uint numLengths, byte[] values)
        {
            var table = new Dictionary<long, byte>();
            uint curIndex = 0;
            ushort code = 0;
            for (uint length = 0; length < numLengths; length++)
            {
                uint numValues = lengths[length];
                for (uint i = 0; i < numValues; i++)
                {
                    long mixed = (long)(length + 1) << 32 | code;
                    table[mixed] = values[curIndex++];
                    code++;
                }
                code <<= 1;
            }

            return table;
        }

        float[] GenerateQuantizationTable(byte qualityFactor, byte[] quantTemplate)
        {
            var table = new float[64];
            float multiplier = (200.0f - qualityFactor * 2.0f) * 0.01f;
            for (int i = 0; i < 64; i++)
            {
                table[i] = quantTemplate[i] * multiplier;
            }
            return table;
        }

        byte[] DecompressMipInternal(int width, int height, BitStream stream, uint format,
            Dictionary<long, byte> dcLuminanceTable, Dictionary<long, byte> acLuminanceTable, Dictionary<long, byte> dcChrominanceTable, Dictionary<long, byte> acChrominanceTable,
            Tex.LayerInfo[] layerInfo, float[][] luminosityQuantization, float[][] chrominanceQuantization)
        {
            byte[] data;
            switch (format)
            {
                case 0:
                    data = DecompressFormat0(width, height, stream, dcLuminanceTable, acLuminanceTable, dcChrominanceTable, acChrominanceTable, luminosityQuantization, chrominanceQuantization);
                    break;
                case 1:
                    data = DecompressFormat1(width, height, stream, dcLuminanceTable, acLuminanceTable, layerInfo, luminosityQuantization);
                    break;
                case 2:
                    data = DecompressFormat2(width, height, stream, dcLuminanceTable, acLuminanceTable, dcChrominanceTable, acChrominanceTable, layerInfo, luminosityQuantization, chrominanceQuantization);
                    break;
                default:
                    Console.WriteLine($"Wrong Compression Format {format}");
                    return null;
            }

            // BGRA to RGBA
            for (var i = 0; i < data.Length; i += 4)
            {
                var r = data[i];
                var b = data[i + 2];
                var rt = r;
                data[i] = b;
                data[i + 2] = rt;
            }

            return data;
        }

        byte[] DecompressFormat0(int width, int height, BitStream stream,
            Dictionary<long, byte> dcLuminanceTable, Dictionary<long, byte> acLuminanceTable, Dictionary<long, byte> dcChrominanceTable, Dictionary<long, byte> acChrominanceTable,
            float[][] luminosityQuantization, float[][] chrominanceQuantization)
        {
            short[][] luminance0 = new short[4][];
            short[][] luminance1 = new short[4][];

            var mipWidth = ((width + 15) / 16) * 16;
            var mipHeight = ((height + 15) / 16) * 16;
            byte[] data = new byte[mipWidth * mipHeight * 4];

            short[] previousDc = new short[4];
            for (var y = 0; y < (mipHeight / 16); y++)
            {
                for (var x = 0; x < (mipWidth / 16); x++)
                {
                    for (int i = 0; i < luminance0.Length; i++)
                    {
                        previousDc[0] = ProcessBlock(previousDc[0], stream, dcLuminanceTable, acLuminanceTable, luminosityQuantization[0], true, out luminance0[i]);
                    }
                    previousDc[1] = ProcessBlock(previousDc[1], stream, dcChrominanceTable, acChrominanceTable, chrominanceQuantization[1], false, out short[] chrominance0);
                    previousDc[2] = ProcessBlock(previousDc[2], stream, dcChrominanceTable, acChrominanceTable, chrominanceQuantization[2], false, out short[] chrominance1);
                    for (int i = 0; i < luminance1.Length; i++)
                    {
                        previousDc[3] = ProcessBlock(previousDc[3], stream, dcLuminanceTable, acLuminanceTable, luminosityQuantization[3], true, out luminance1[i]);
                    }

                    byte[][] colors = DecodeColorBlockFormat0(luminance0, luminance1, chrominance0, chrominance1);
                    for (var row = 0; row < 16; row++)
                    {
                        if (y * 16 + row >= height || x * 16 >= width)
                        {
                            continue;
                        }

                        var numPixels = Math.Min(16u, width - x * 16);

                        for (int i = 0; i < numPixels; i++)
                        {
                            data[(y * 16 + row) * width * 4 + x * 16 * 4 + (i * 4) + 0] = colors[row * 16 + i][0];
                            data[(y * 16 + row) * width * 4 + x * 16 * 4 + (i * 4) + 1] = colors[row * 16 + i][1];
                            data[(y * 16 + row) * width * 4 + x * 16 * 4 + (i * 4) + 2] = colors[row * 16 + i][2];
                            data[(y * 16 + row) * width * 4 + x * 16 * 4 + (i * 4) + 3] = colors[row * 16 + i][3];
                        }
                    }
                }
            }

            return data;
        }

        byte[] DecompressFormat1(int width, int height, BitStream stream,
            Dictionary<long, byte> dcLuminanceTable, Dictionary<long, byte> acLuminanceTable,
            Tex.LayerInfo[] layerInfo, float[][] luminosityQuantization)
        {
            short[][] luminance = new short[4][]; // [4][64]

            var mipWidth = ((width + 7) / 8) * 8;
            var mipHeight = ((height + 7) / 8) * 8;
            byte[] data = new byte[mipWidth * mipHeight * 4];

            short[] previousDc = new short[4];
            for (var y = 0; y < (mipHeight / 8); y++)
            {
                for (var x = 0; x < (mipWidth / 8); x++)
                {
                    previousDc[0] = ProcessBlock(previousDc[0], stream, dcLuminanceTable, acLuminanceTable, luminosityQuantization[0], true, out luminance[0]);
                    previousDc[1] = ProcessBlock(previousDc[1], stream, dcLuminanceTable, acLuminanceTable, luminosityQuantization[1], true, out luminance[1]);
                    if (layerInfo[2].hasReplacement == 0)
                    {
                        previousDc[2] = ProcessBlock(previousDc[2], stream, dcLuminanceTable, acLuminanceTable, luminosityQuantization[2], true, out luminance[2]);
                    }
                    if (layerInfo[3].hasReplacement == 0)
                    {
                        previousDc[3] = ProcessBlock(previousDc[3], stream, dcLuminanceTable, acLuminanceTable, luminosityQuantization[3], true, out luminance[3]);
                    }

                    for (var iy = 0; iy < 8; iy++)
                    {
                        for (var ix = 0; ix < 8; ix++)
                        {
                            var r = (byte)(luminance[0][iy * 8 + ix]);
                            var g = (byte)(luminance[1][iy * 8 + ix]);
                            var b = (byte)255;
                            if (layerInfo[2].hasReplacement == 0)
                                b = (byte)(luminance[2][iy * 8 + ix]);
                            var a = (byte)255;
                            if (layerInfo[3].hasReplacement == 0)
                                a = (byte)(luminance[3][iy * 8 + ix]);
                            data[((y * 8 + iy) * width + (x * 8 + ix)) * 4] = b;
                            data[((y * 8 + iy) * width + (x * 8 + ix)) * 4 + 1] = g;
                            data[((y * 8 + iy) * width + (x * 8 + ix)) * 4 + 2] = a;
                            data[((y * 8 + iy) * width + (x * 8 + ix)) * 4 + 3] = r;
                        }
                    }
                }
            }

            return data;
        }

        byte[] DecompressFormat2(int width, int height, BitStream stream,
            Dictionary<long, byte> dcLuminanceTable, Dictionary<long, byte> acLuminanceTable, Dictionary<long, byte> dcChrominanceTable, Dictionary<long, byte> acChrominanceTable,
            Tex.LayerInfo[] layerInfo, float[][] luminosityQuantization, float[][] chrominanceQuantization)
        {

            var mipWidth = ((width + 7) / 8) * 8;
            var mipHeight = ((height + 7) / 8) * 8;
            byte[] data = new byte[mipWidth * mipHeight * 4];

            short[] previousDc = new short[4];
            for (var y = 0; y < (mipHeight / 8); y++)
            {
                for (var x = 0; x < (mipWidth / 8); x++)
                {
                    previousDc[0] = ProcessBlock(previousDc[0], stream, dcLuminanceTable, acLuminanceTable, luminosityQuantization[0], true, out short[] luminance0);
                    previousDc[1] = ProcessBlock(previousDc[1], stream, dcChrominanceTable, acChrominanceTable, chrominanceQuantization[1], false, out short[] chrominance0);
                    previousDc[2] = ProcessBlock(previousDc[2], stream, dcChrominanceTable, acChrominanceTable, chrominanceQuantization[2], false, out short[] chrominance1);

                    short[] luminance1 = null;
                    if (layerInfo[3].hasReplacement == 0)
                    {
                        previousDc[3] = ProcessBlock(previousDc[3], stream, dcLuminanceTable, acLuminanceTable, luminosityQuantization[3], true, out luminance1);
                    }

                    byte[][] colors = DecodeColorBlockFormat2(luminance0, luminance1, chrominance0, chrominance1);
                    for (var row = 0; row < 8; row++)
                    {
                        if ((y * 8 + row) >= height)
                        {
                            continue;
                        }

                        if (x * 8 >= width)
                        {
                            continue;
                        }

                        var numPixels = Math.Min(8u, width - x * 8);

                        for (int i = 0; i < numPixels; i++)
                        {
                            data[(y * 8 + row) * width * 4 + x * 8 * 4 + (i * 4) + 0] = colors[row * 8 + i][0];
                            data[(y * 8 + row) * width * 4 + x * 8 * 4 + (i * 4) + 1] = colors[row * 8 + i][1];
                            data[(y * 8 + row) * width * 4 + x * 8 * 4 + (i * 4) + 2] = colors[row * 8 + i][2];
                            data[(y * 8 + row) * width * 4 + x * 8 * 4 + (i * 4) + 3] = colors[row * 8 + i][3];
                        }
                    }
                }
            }

            return data;
        }

        byte[][] DecodeColorBlockFormat0(short[][] lum0, short[][] lum1, short[] chrom0, short[] chrom1)
        {
            byte[][] colors = new byte[16 * 16][];
            for (uint y = 0; y < 16; y++)
            {
                for (uint x = 0; x < 16; x++)
                {
                    uint cy = (y >= 8) ? 1u : 0u;
                    uint cx = (x >= 8) ? 1u : 0u;
                    uint by = y % 8;
                    uint bx = x % 8;

                    uint block = cy * 2 + cx;
                    uint lumIdx = by * 8 + bx;
                    uint crmIdx = (y / 2) * 8 + (x / 2);
                    colors[y * 16 + x] = YCbCrToColor32(lum0[block][lumIdx], lum1[block][lumIdx], chrom0[crmIdx], chrom1[crmIdx]);
                }
            }
            return colors;
        }

        byte[][] DecodeColorBlockFormat2(short[] lum0, short[] lum1, short[] chrom0, short[] chrom1)
        {
            byte[][] colors = new byte[8 * 8][];
            for (uint y = 0; y < 8; y++)
            {
                for (uint x = 0; x < 8; x++)
                {
                    uint idx = y * 8 + x;
                    short alpha = 1;
                    if (lum1 != null)
                        alpha = lum1[idx];
                    colors[idx] = YCbCrToColor32(lum0[idx], alpha, chrom0[idx], chrom1[idx]);
                }
            }

            return colors;
        }

        /*
        byte[] YCbCrToColor32(int y, int lum1, int cb, int cr)
        {
            int a = y - (cr >> 1);
            var beta = (byte)(a + cr);
            var gamma = (byte)(a - (cb >> 1));
            var delta = (byte)(gamma + cb);
            var alpha = (byte)(lum1);
            return new byte[] { gamma, beta, delta, alpha };
        }
        */

        byte[] YCbCrToColor32(int y, int lum1, int cb, int cr)
        {
            int a = y - (cr >> 1);
            var beta = (byte)Clamp(a + cr, 0, 0xFF);
            var gamma = (byte)Clamp(a - (cb >> 1), 0, 0xFF);
            var delta = (byte)Clamp(gamma + cb, 0, 0xFF);
            var alpha = (byte)Clamp(lum1, 0, 0xFF);
            return new byte[] { gamma, beta, delta, alpha };
        }

        short ProcessBlock(short previousDc, BitStream stream, Dictionary<long, byte> dcTable, Dictionary<long, byte> acTable, float[] quantizationTable, bool isLuminance, out short[] output)
        {
            output = new short[64];
            short[] bufferBlock = new short[64];

            short currentDc = DecodeBlock(stream, bufferBlock, dcTable, acTable, previousDc);
            UnZigZag(bufferBlock);
            Dequantize(bufferBlock, quantizationTable);
            //IDCT.Calculate(bufferBlock);
            IDCT2.DoIdct(bufferBlock);

            for (var i = 0; i < 64; i++)
            {
                short value = bufferBlock[i];
                if (isLuminance)
                {
                    value = (short)Clamp(value + 128, 0, 255);
                }
                else
                {
                    value = (short)Clamp(value, -256, 255);
                }
                output[i] = value;
            }

            return currentDc;
        }

        short DecodeBlock(BitStream stream, short[] block, Dictionary<long, byte> dcTable, Dictionary<long, byte> acTable, short previousDc = 0)
        {
            var dcLength = DecodeValue(stream, dcTable);
            var epsilon = stream.ReadBits(dcLength);
            var deltaDC = Extend(epsilon, dcLength);
            short currentDC = (short)(deltaDC + previousDc);

            block[0] = currentDC;

            for (var idx = 1; idx < 64;)
            {
                var acCodedValue = DecodeValue(stream, acTable);
                if (acCodedValue == 0)
                {
                    break;
                }

                if (acCodedValue == 0xF0)
                {
                    idx += 16;
                    continue;
                }

                idx += (acCodedValue >> 4) & 0xF;
                var acLength = (byte)(acCodedValue & 0xF);
                epsilon = stream.ReadBits(acLength);
                var acValue = Extend(epsilon, acLength);
                block[idx] = acValue;
                idx++;
            }

            return currentDC;
        }

        byte DecodeValue(BitStream stream, Dictionary<long, byte> table)
        {
            ushort word = 0;
            byte wordLength = 0;
            do
            {
                try
                {
                    word <<= 1;
                    word |= stream.ReadBit();
                    wordLength++;

                    long mixed = (long)wordLength << 32 | word;
                    if (table.TryGetValue(mixed, out byte itr))
                    {
                        return itr;
                    }
                }
                catch
                {
                    return 0;
                }
            }
            while (wordLength < 16);

            Console.WriteLine("Huffman Table doesn't contain word");
            return 0;
        }

        short Extend(ushort value, ushort length)
        {
            if (value < (1 << (length - 1)))
            {
                return (short)(value + (-1 << length) + 1);
            }
            else
            {
                return (short)value;
            }
        }

        void UnZigZag(short[] block)
        {
            short[] buffer = new short[64];
            for (var i = 0; i < 64; i++)
            {
                buffer[i] = block[zigzagMapping[i]];
            }

            System.Array.Copy(buffer, block, 64);
        }

        void Dequantize(short[] block, float[] quantizationTable)
        {
            for (var i = 0; i < 64; i++)
            {
                block[i] = (short)(block[i] * quantizationTable[i]);
            }
        }

        public static int Clamp(int value, int min, int max)
        {
            return (value < min) ? min : (value > max) ? max : value;
        }
    }
}