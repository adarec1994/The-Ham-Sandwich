using MathUtils;
using ProjectWS.FileFormats.Common;


namespace ProjectWS.FileFormats.I3
{
    public class Portal : ArrayData
    {
        public uint nameIndex;
        public ushort sectorIndexA;
        public ushort sectorIndexB;
        public AABB? boundsA;
        public Plane plane;
        public Vector3[]? vertices;
        public ushort[]? indices;
        public AABB? boundsB;
        public float side;
        public float unk1;

        public override int GetSize()
        {
            return 144;
        }

        public override void Read(BinaryReader br, long endOffset = 0)
        {
            this.nameIndex = br.ReadUInt32();
            this.sectorIndexA = br.ReadUInt16();
            this.sectorIndexB = br.ReadUInt16();
            br.BaseStream.Position += 8;
            this.boundsA = new AABB(br);
            this.plane = new Plane(br);
            this.vertices = new ArrayVector3(br, endOffset).data;
            this.indices = new ArrayUInt16(br, endOffset).data;
            this.boundsB = new AABB(br);
            br.BaseStream.Position += 8;
            this.side = br.ReadSingle();
            this.unk1 = br.ReadSingle();
        }
    }
}
