using MathUtils;
using ProjectWS.FileFormats.Extensions;

namespace ProjectWS.FileFormats.Area
{
    public struct Curt
    {
        public int unk0;
        public short positionCount;
        public Placement placement;
        public short unk5;
        public int positionOffset;
        public int unk6;
        public Vector3[] positions;

        public Curt(BinaryReader br, long chunkStart)
        {
            this.unk0 = br.ReadInt32();
            this.positionCount = br.ReadInt16();
            this.placement = new Placement(br);
            this.unk5 = br.ReadInt16();
            this.positionOffset = br.ReadInt32();
            this.unk6 = br.ReadInt32();

            var save = br.BaseStream.Position;
            br.BaseStream.Position = chunkStart + this.positionOffset;
            this.positions = new Vector3[this.positionCount];
            for (int i = 0; i < this.positionCount; i++)
            {
                this.positions[i] = br.ReadVector3();
            }
            br.BaseStream.Position = save;
        }
    }
}
