using System.Collections;
using System.Collections.Generic;

namespace ProjectWS.FileFormats.Tex.Jpeg
{
    public class BitStream
    {
        byte[] mData;
        int mBitPosition = 0;

        public BitStream(byte[] data)
        {
            this.mData = data;
        }

        public byte ReadBit()
        {
            int bytePosition = mBitPosition / 8;
            int bitPosition = mBitPosition % 8;
            bitPosition = 7 - bitPosition;

            var ret = (byte)((mData[bytePosition] & (1 << bitPosition)) >> bitPosition);
            ++mBitPosition;
            return ret;
        }

        public ushort ReadBits(byte numBits)
        {
            if (numBits >= 16)
            {
                Console.WriteLine($"Attempted to read {numBits} bits which is more than maximum 16");
                throw new System.Exception("Invalid bit count");
            }

            ushort ret = 0;
            for (byte i = 0; i < numBits; ++i)
            {
                ret <<= 1;
                ret |= ReadBit();
            }

            return ret;
        }
    }
}
