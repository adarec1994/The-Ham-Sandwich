using ProjectWS.FileFormats.Common;

namespace ProjectWS.FileFormats.Sho
{
    public class Variant : ArrayData
    {
        public byte[]? data;

        public override void Read(BinaryReader br, long startOffset)
        {
            //Debug.Log(startOffset);
            this.data = new ArrayByte(br, startOffset).data;
        }

        public override int GetSize()
        {
            return 16;
        }
    }
}
