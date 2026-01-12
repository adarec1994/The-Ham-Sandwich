namespace ProjectWS.FileFormats.Area
{
    public partial class Water
    {
        public uint worldWaterTypeID;           // Entry in WorldWaterType tbl
        public uint[] waterLayerIDs;            // Entries in WorldWaterLayer tbl, they get blended together (vertex layer blend mask)
        public uint unk0;
        public float unk1;
        public uint unk2;
        public float unk3;
        public float unk4;
        public uint unk5;
        public uint unk6;
        public float shoreLineDistance;
        public float unk7;
        public uint shoreLineWaterLayerID;      // Entry in WorldWaterLayer tbl
        public uint unk8;
        public uint indexCount;
        public uint vertexCount;
        public uint unk9;
        public uint unk10;

        public WaterVertex[]? vertices;
        public uint[]? indexData;

        public Water(BinaryReader br)
        {
            this.worldWaterTypeID = br.ReadUInt32();
            this.waterLayerIDs = new uint[4];
            for (int i = 0; i < 4; i++)
            {
                this.waterLayerIDs[i] = br.ReadUInt32();
            }
            this.unk0 = br.ReadUInt32();
            this.unk1 = br.ReadSingle();
            this.unk2 = br.ReadUInt32();
            this.unk3 = br.ReadSingle();
            this.unk4 = br.ReadSingle();
            this.unk5 = br.ReadUInt32();
            this.unk6 = br.ReadUInt32();
            this.shoreLineDistance = br.ReadSingle();
            this.unk7 = br.ReadSingle();
            this.shoreLineWaterLayerID = br.ReadUInt32();
            this.unk8 = br.ReadUInt32();
            this.indexCount = br.ReadUInt32();
            this.vertexCount = br.ReadUInt32();
            this.unk9 = br.ReadUInt32();
            this.unk10 = br.ReadUInt32();

            this.indexData = new uint[this.indexCount];
            for (int i = 0; i < this.indexCount; i++)
            {
                this.indexData[i] = br.ReadUInt32();
            }

            this.vertices = new WaterVertex[this.vertexCount];

            for (int i = 0; i < this.vertexCount; i++)
            {
                this.vertices[i] = new WaterVertex(br);
            }
        }
    }
}
