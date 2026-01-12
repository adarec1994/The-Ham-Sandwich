using MathUtils;
using ProjectWS.FileFormats.Common;


namespace ProjectWS.FileFormats.I3
{
    public class PassUnk0 : ArrayData
    {
        public uint unk0;
        public uint unk1;
        public uint unk2;
        public uint unk3;
        public uint unk4;
        public PassUnk0A[]? arrUnk0A;

        public override int GetSize()
        {
            return 40;
        }

        public override void Read(BinaryReader br, long endOffset = 0)
        {
            br.BaseStream.Position += 4;
            this.unk0 = br.ReadUInt32();
            this.unk1 = br.ReadUInt32();
            this.unk2 = br.ReadUInt32();
            this.unk3 = br.ReadUInt32();
            this.unk4 = br.ReadUInt32();
            this.arrUnk0A = new ArrayStruct<PassUnk0A>(br, endOffset, true).data;
        }
    }
}
