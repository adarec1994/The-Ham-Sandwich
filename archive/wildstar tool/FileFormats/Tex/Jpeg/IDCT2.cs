using System;
using System.Collections;
using System.Collections.Generic;
using System.IO;


namespace ProjectWS.FileFormats.Tex.Jpeg
{
    public static class Extensions
    {
        public static void Swap<T>(this IList<T> arr, int i1, int i2)
        {
            T tempT = arr[i1];
            arr[i1] = arr[i2];
            arr[i2] = tempT;
        }

        public static Int16 ReadInt16BE(this BinaryReader reader)
        {
            byte[] temp = reader.ReadBytes(2);
            return (short)(
                temp[0] << 8 |
                temp[1]
            );
        }
    }

    static class IDCT2
    {
        const int Side = 8;

        private static readonly float[] Dct = GenerateDct();
        private static readonly float[] DctT = Transpose(GenerateDct());

        private static float[] GenerateDct()
        {
            const int Size2 = Side * Side;

            float[] result = new float[Size2];
            for (int y = 0, o = 0; y < Side; y++)
                for (int x = 0; x < Side; x++)
                    result[o++] = (float)(Math.Sqrt(y == 0 ? .125f : .250f) * Math.Cos(((2 * x + 1) * y * Math.PI) * .0625f));

            return result;
        }

        private static float[] Transpose(float[] m)
        {

            for (int y = 0; y < Side; y++)
                for (int x = y + 1; x < Side; x++)
                    m.Swap(y * Side + x, x * Side + y);

            return m;
        }

        private static float[] MatrixMultiply(float[] m1, float[] m2)
        {

            float[] result = new float[m1.Length];
            for (int y = 0; y < Side; y++)
                for (int x = 0; x < Side; x++)
                {
                    float sum = 0;
                    for (int k = 0; k < Side; k++)
                        sum += m1[y * Side + k] * m2[k * Side + x];
                    result[y * Side + x] = sum;
                }

            return result;
        }

        public static float[] ToFloat(short[] m)
        {
            float[] r = new float[m.Length];
            for (int i = 0; i < m.Length; i++)
                r[i] = (float)m[i];
            return r;
        }

        public static void DoIdct(short[] m)
        {
            float[] source = ToFloat(m);

            source = MatrixMultiply(DctT, source);
            source = MatrixMultiply(source, Dct);

            short[] r = new short[m.Length];

            for (int i = 0; i < m.Length; i++)
                m[i] = (short)Math.Round(source[i]);
        }

    }
}