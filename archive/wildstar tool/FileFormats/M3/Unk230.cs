using ProjectWS.FileFormats.Common;

namespace ProjectWS.FileFormats.M3
{
    public class Unk230 : ArrayData
    {
        public byte[]? data;

        public override int GetSize()
        {
            return 4;
        }

        public override void Read(BinaryReader br, long startOffset)
        {
            this.data = br.ReadBytes(4);
        }
    }
}