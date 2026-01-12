
namespace ProjectWS.FileFormats.Tex.Jpeg
{
    /// <summary>
    /// IDCT - Inverse discrete cosine transform
    /// https://en.wikipedia.org/wiki/Discrete_cosine_transform
    /// </summary>
    public unsafe class IDCT
    {
        public static bool initialized;

        static short[] iclip = new short[1024];
        static short* iclp;

        const int W1 = 2841;
        const int W2 = 2676;
        const int W3 = 2408;
        const int W5 = 1609;
        const int W6 = 1108;
        const int W7 = 565;

        public static void Initialize()
        {
            if (initialized) return;

            int i;

            fixed (short* iclipp = iclip)
            {
                iclp = iclipp + 512;
                for (i = -512; i < 512; i++)
                    iclp[i] = (short)((i < -256) ? -256 : ((i > 255) ? 255 : i));
            }

            initialized = true;
        }

        static void IDCTRow(short* blk)
        {
            int x0, x1, x2, x3, x4, x5, x6, x7, x8;

            if (!(((x1 = blk[4] << 11) | (x2 = blk[6]) | (x3 = blk[2]) | (x4 = blk[1]) | (x5 = blk[7]) | (x6 = blk[5]) | (x7 = blk[3])) != 0))
            {
                var c = (short)(blk[0] << 3);
                blk[0] = c;
                blk[1] = c;
                blk[2] = c;
                blk[3] = c;
                blk[4] = c;
                blk[5] = c;
                blk[6] = c;
                blk[7] = c;
                return;
            }

            x0 = (blk[0] << 11) + 128;

            x8 = W7 * (x4 + x5);
            x4 = x8 + (W1 - W7) * x4;
            x5 = x8 - (W1 + W7) * x5;
            x8 = W3 * (x6 + x7);
            x6 = x8 - (W3 - W5) * x6;
            x7 = x8 - (W3 + W5) * x7;

            x8 = x0 + x1;
            x0 -= x1;
            x1 = W6 * (x3 + x2);
            x2 = x1 - (W2 + W6) * x2;
            x3 = x1 + (W2 - W6) * x3;
            x1 = x4 + x6;
            x4 -= x6;
            x6 = x5 + x7;
            x5 -= x7;

            x7 = x8 + x3;
            x8 -= x3;
            x3 = x0 + x2;
            x0 -= x2;
            x2 = (181 * (x4 + x5) + 128) >> 8;
            x4 = (181 * (x4 - x5) + 128) >> 8;

            blk[0] = (short)((x7 + x1) >> 8);
            blk[1] = (short)((x3 + x2) >> 8);
            blk[2] = (short)((x0 + x4) >> 8);
            blk[3] = (short)((x8 + x6) >> 8);
            blk[4] = (short)((x8 - x6) >> 8);
            blk[5] = (short)((x0 - x4) >> 8);
            blk[6] = (short)((x3 - x2) >> 8);
            blk[7] = (short)((x7 - x1) >> 8);
        }

        static void IDCTColumn(short* blk)
        {
            int x0, x1, x2, x3, x4, x5, x6, x7, x8;

            if (!(((x1 = (blk[8 * 4] << 8)) | (x2 = blk[8 * 6]) | (x3 = blk[8 * 2]) | (x4 = blk[8 * 1]) | (x5 = blk[8 * 7]) | (x6 = blk[8 * 5]) | (x7 = blk[8 * 3])) != 0))
            {
                var c = iclp[((blk[8 * 0] + 32) >> 6)];
                blk[8 * 0] = c;
                blk[8 * 1] = c;
                blk[8 * 2] = c;
                blk[8 * 3] = c;
                blk[8 * 4] = c;
                blk[8 * 5] = c;
                blk[8 * 6] = c;
                blk[8 * 7] = c;
                return;
            }

            x0 = (blk[8 * 0] << 8) + 8192;

            x8 = W7 * (x4 + x5) + 4;
            x4 = (x8 + (W1 - W7) * x4) >> 3;
            x5 = (x8 - (W1 + W7) * x5) >> 3;
            x8 = W3 * (x6 + x7) + 4;
            x6 = (x8 - (W3 - W5) * x6) >> 3;
            x7 = (x8 - (W3 + W5) * x7) >> 3;

            x8 = x0 + x1;
            x0 -= x1;
            x1 = W6 * (x3 + x2) + 4;
            x2 = (x1 - (W2 + W6) * x2) >> 3;
            x3 = (x1 + (W2 - W6) * x3) >> 3;
            x1 = x4 + x6;
            x4 -= x6;
            x6 = x5 + x7;
            x5 -= x7;

            x7 = x8 + x3;
            x8 -= x3;
            x3 = x0 + x2;
            x0 -= x2;
            x2 = (181 * (x4 + x5) + 128) >> 8;
            x4 = (181 * (x4 - x5) + 128) >> 8;

            blk[8 * 0] = iclp[(x7 + x1) >> 14];
            blk[8 * 1] = iclp[(x3 + x2) >> 14];
            blk[8 * 2] = iclp[(x0 + x4) >> 14];
            blk[8 * 3] = iclp[(x8 + x6) >> 14];
            blk[8 * 4] = iclp[(x8 - x6) >> 14];
            blk[8 * 5] = iclp[(x0 - x4) >> 14];
            blk[8 * 6] = iclp[(x3 - x2) >> 14];
            blk[8 * 7] = iclp[(x7 - x1) >> 14];
        }

        public static void Calculate(short[] workBlock)
        {
            int i;
            fixed (short* block = workBlock)
            {
                for (i = 0; i < 8; i++)
                    IDCTRow(block + 8 * i);

                for (i = 0; i < 8; i++)
                    IDCTColumn(block + i);
            }
        }
    }
}