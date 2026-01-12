using MathUtils;
using ProjectWS.FileFormats.Common;

namespace ProjectWS.FileFormats.M3
{
    public class Bounds : ArrayData
    {
        public short unk0;         // 45 .. 5000+
        public short unk1;         // 0 .. 31
        public short unk2;         // 0, 1, 2, 3
        public short unk3;         // 0, 1, 3, 4
        public short unk4;         // 4 .. 952
        public short unk5;         // 1 .. 10000
        public short unk6;         // ...
        public short unk7;         // 0 .. 93
        public short unk8;         // ...
        public short unk9;         // 0 .. 93
        public float unk10;
        public short unk11;
        public short unk12;
        public short unk13;
        public short unk14;
        public AABB bbA;
        public AABB bbB;
        public string? name;

        public override int GetSize()
        {
            return 112;
        }

        public override void Read(BinaryReader br, long startOffset)
        {
            this.unk0 = br.ReadInt16();
            this.unk1 = br.ReadInt16();
            this.unk2 = br.ReadInt16();
            this.unk3 = br.ReadInt16();
            this.unk4 = br.ReadInt16();
            this.unk5 = br.ReadInt16();
            this.unk6 = br.ReadInt16();
            this.unk7 = br.ReadInt16();
            this.unk8 = br.ReadInt16();
            this.unk9 = br.ReadInt16();
            this.unk10 = br.ReadSingle();
            this.unk11 = br.ReadInt16();
            this.unk12 = br.ReadInt16();
            this.unk13 = br.ReadInt16();
            this.unk14 = br.ReadInt16();
            this.bbA = new AABB(br);
            this.bbB = new AABB(br);
            this.name = new string(new ArrayWChar(br, startOffset).data);
        }
    }
}