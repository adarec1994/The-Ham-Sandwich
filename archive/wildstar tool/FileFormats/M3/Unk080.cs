using ProjectWS.FileFormats.Common;

namespace ProjectWS.FileFormats.M3
{
    public class Unk080 : ArrayData
    {
        public short[]? unks;    // Probably wrong

        public override int GetSize()
        {
            return 16 + 32;
        }

        public override void Read(BinaryReader br, long startOffset)
        {
            this.unks = new short[8];
            for (int i = 0; i < 8; i++)
            {
                this.unks[i] = br.ReadInt16();
            }
            br.BaseStream.Position += 32;   // Padding ?
        }
    }
}