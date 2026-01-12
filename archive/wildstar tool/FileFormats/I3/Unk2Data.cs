using MathUtils;
using ProjectWS.FileFormats.Common;


namespace ProjectWS.FileFormats.I3
{
    public class Unk2Data : ArrayData
    {
        public int unk0;
        public int unk1;
        public Vector3 position;
        public Vector3 scale;
        public Quaternion rotation;

        public override int GetSize()
        {
            return 64;
        }

        public override void Read(BinaryReader br, long endOffset = 0)
        {
            this.unk0 = br.ReadInt32();
            this.unk1 = br.ReadInt32();
            br.BaseStream.Position += 8;
            this.position = new Vector3(br.ReadSingle(), br.ReadSingle(), br.ReadSingle());
            br.BaseStream.Position += 4;
            this.scale = new Vector3(br.ReadSingle(), br.ReadSingle(), br.ReadSingle());
            br.BaseStream.Position += 4;
            this.rotation = new Quaternion(br.ReadSingle(), br.ReadSingle(), br.ReadSingle(), br.ReadSingle());
        }
    }
}
