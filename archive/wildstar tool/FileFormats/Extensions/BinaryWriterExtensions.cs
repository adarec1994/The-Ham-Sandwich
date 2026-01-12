using System.Text;

namespace ProjectWS.FileFormats.Extensions
{
    public static class BinaryWriterExtensions
    {
        public static int WriteWString(this BinaryWriter bw, string str)
        {
            var data = Encoding.Unicode.GetBytes(str);
            bw.Write(data);
            return data.Length;
        }

        public static void MoveTo(this BinaryWriter bw, long position)
        {
            if (bw.BaseStream.Length > position)
            {
                bw.BaseStream.Position = position;
            }
            else
            {
                long allocate = position - (bw.BaseStream.Length - 1);
                bw.BaseStream.Position = bw.BaseStream.Length - 1;
                bw.Write(new byte[allocate]);
            }
        }

        public static void Align(this BinaryWriter bw, int alignment = 4)
        {
            int loc = (int)bw.BaseStream.Position;
            int pad = (alignment - (loc % alignment));
            if (pad != 0 && pad < alignment)
            {
                bw.Write(new byte[pad]);
            }
        }
    }
}
