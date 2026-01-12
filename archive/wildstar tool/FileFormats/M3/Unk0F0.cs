using ProjectWS.FileFormats.Common;

namespace ProjectWS.FileFormats.M3
{
    public class Unk0F0 : ArrayData
    {
        public short[]? unks;

        public override int GetSize()
        {
            return 92 * 2;
        }

        public override void Read(BinaryReader br, long startOffset)
        {
            this.unks = new short[92];
            for (int i = 0; i < 92; i++)
            {
                this.unks[i] = br.ReadInt16();
            }
        }
    }
}