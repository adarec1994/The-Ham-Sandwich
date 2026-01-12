using ProjectWS.FileFormats.Common;

namespace ProjectWS.FileFormats.M3
{
    public class Model2Display : ArrayData
    {
        public ushort modelMeshID;
        public ushort default2Render;

        public override int GetSize()
        {
            return 4;
        }

        public override void Read(BinaryReader br, long startOffset)
        {
            this.modelMeshID = br.ReadUInt16();
            this.default2Render = br.ReadUInt16();
        }
    }
}