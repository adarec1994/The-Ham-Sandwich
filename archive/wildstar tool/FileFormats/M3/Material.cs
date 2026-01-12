using ProjectWS.FileFormats.Common;

namespace ProjectWS.FileFormats.M3
{
    public class Material : ArrayData
    {
        public int unk0;
        public int unk1;
        public int unk2;
        public int unk3;
        public int unk4;
        public int unk5;
        public int unk6;
        public int unk7;
        public MaterialDescription[]? materialDescriptions;

        public override void Read(BinaryReader br, long endOffset)
        {
            this.unk0 = br.ReadInt32();
            this.unk1 = br.ReadInt32();
            this.unk2 = br.ReadInt32();
            this.unk3 = br.ReadInt32();
            this.unk4 = br.ReadInt32();
            this.unk5 = br.ReadInt32();
            this.unk6 = br.ReadInt32();
            this.unk7 = br.ReadInt32();
            this.materialDescriptions = new ArrayStruct<MaterialDescription>(br, endOffset).data;
        }

        public override int GetSize() => 48;
    }
}