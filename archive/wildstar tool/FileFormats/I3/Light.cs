using MathUtils;
using ProjectWS.FileFormats.Common;


namespace ProjectWS.FileFormats.I3
{
    public class Light : ArrayData
    {
        public int nameIndex;
        public Vector3 position;
        public Vector3 scale;
        public Quaternion rotation;
        public int unk0;
        public int unk1;
        public int unk2;
        public int unk3;
        public Vector4 unkVec0;
        public Vector4 unkVec1;
        public Vector4 unkVec2;
        public Vector4 unkVec3;

        public override int GetSize()
        {
            return 176;
        }

        public override void Read(BinaryReader br, long endOffset = 0)
        {
            this.nameIndex = br.ReadInt32();
            br.BaseStream.Position += 12;
            this.position = new Vector3(br.ReadSingle(), br.ReadSingle(), br.ReadSingle());
            br.BaseStream.Position += 4;
            this.scale = new Vector3(br.ReadSingle(), br.ReadSingle(), br.ReadSingle());
            br.BaseStream.Position += 4;
            this.rotation = new Quaternion(br.ReadSingle(), br.ReadSingle(), br.ReadSingle(), br.ReadSingle());
            this.unk0 = br.ReadInt32();
            this.unk1 = br.ReadInt32();
            this.unk2 = br.ReadInt32();
            this.unk3 = br.ReadInt32();
            br.BaseStream.Position += 16;
            this.unkVec0 = new Vector4(br.ReadSingle(), br.ReadSingle(), br.ReadSingle(), br.ReadSingle());
            this.unkVec1 = new Vector4(br.ReadSingle(), br.ReadSingle(), br.ReadSingle(), br.ReadSingle());
            this.unkVec2 = new Vector4(br.ReadSingle(), br.ReadSingle(), br.ReadSingle(), br.ReadSingle());
            this.unkVec3 = new Vector4(br.ReadSingle(), br.ReadSingle(), br.ReadSingle(), br.ReadSingle());
            br.BaseStream.Position += 16;
        }
    }
}
