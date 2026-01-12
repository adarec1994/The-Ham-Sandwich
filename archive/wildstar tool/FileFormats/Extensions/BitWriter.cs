using System.Collections;

namespace ProjectWS.FileFormats.Extensions
{
    public class BitWriter
    {
        public BinaryWriter bw;
        public byte curBitIndx = 0;
        private bool[] curByte = new bool[8];
        private System.Collections.BitArray? ba;

        public BitWriter(MemoryStream ms)
        {
            this.bw = new BinaryWriter(ms);
        }

        public void Flush()
        {
            this.bw.Write(ConvertToByte(this.curByte));
            this.curBitIndx = 0;
            this.curByte = new bool[8];
        }

        public void Write(bool value)
        {
            this.curByte[this.curBitIndx] = value;
            this.curBitIndx++;

            if (this.curBitIndx == 8)
            {
                Flush();
            }
        }

        public void WriteByte(byte value, int bits)
        {
            this.ba = new BitArray(new byte[] { value });
            for (byte i = 0; i < bits; i++)
            {
                this.Write(this.ba[i]);
            }
            this.ba = null;
        }

        public void WriteBytes(byte[] buffer)
        {
            for (int i = 0; i < buffer.Length; i++)
            {
                this.WriteByte((byte)buffer[i], 8);
            }
        }

        public void WriteUInt16(ushort value, int bits)
        {
            this.ba = new BitArray(BitConverter.GetBytes(value));
            for (byte i = 0; i < bits; i++)
            {
                this.Write(this.ba[i]);
            }
            this.ba = null;
        }

        public void WriteUInt64(ulong value, int bits)
        {
            this.ba = new BitArray(BitConverter.GetBytes(value));
            for (byte i = 0; i < bits; i++)
            {
                this.Write(this.ba[i]);
            }
            this.ba = null;
        }

        public void WriteWString(string value)
        {
            this.Write(true);
            this.WriteUInt16((ushort)(value.Length + 1), 15);

            foreach (char c in value)
            {
                var u = Convert.ToUInt16(c);
                this.WriteUInt16(u, 16);
                //var bytes = System.Text.Encoding.Unicode.GetBytes(new char[] { c });
                //this.WriteBytes(bytes);
            }

            this.WriteUInt16(Convert.ToUInt16('\0'), 16);
        }

        private byte ConvertToByte(bool[] bools)
        {
            byte b = 0;

            byte bitIndex = 0;
            for (int i = 0; i < 8; i++)
            {
                if (bools[i])
                {
                    b |= (byte)(((byte)1) << bitIndex);
                }
                bitIndex++;
            }

            return b;
        }
    }
}
