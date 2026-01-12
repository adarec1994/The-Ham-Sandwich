using MathUtils;
using ProjectWS.FileFormats.Common;

namespace ProjectWS.FileFormats.I3
{
    public class Sector : ArrayData
    {
        public ulong nameIndex;
        public AABB? bb;
        public uint unk;

        public override int GetSize()
        {
            return 64;
        }

        public override void Read(BinaryReader br, long endOffset = 0)
        {
            this.nameIndex = br.ReadUInt64();
            br.BaseStream.Position += 8;
            this.bb = new AABB(br);
            this.unk = br.ReadUInt32();
            br.BaseStream.Position += 12;
        }
    }
}
