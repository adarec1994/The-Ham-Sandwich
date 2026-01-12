using ProjectWS.FileFormats.Common;

namespace ProjectWS.FileFormats.Sky
{
    public class UnkData : ArrayData
    {
        public int[]? unks;
        public override void Read(BinaryReader br, long startOffs)
        {
            this.unks = new int[14];
            for (int i = 0; i < 14; i++)
            {
                this.unks[i] = br.ReadInt32();
            }
        }

        public override int GetSize()
        {
            return 56;
        }
    }
}
