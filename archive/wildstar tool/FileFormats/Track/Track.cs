namespace ProjectWS.FileFormats.Common
{
    public abstract class Track<T>
    {
        public uint elements;
        public uint unused;
        public long offsetTimestamps;
        public long offsetData;

        public ushort[]? timestamps;
        public ushort[]? animationIDs;
        public T[]? data;

        public long ReadTrackCommon(BinaryReader br, long startOffset)
        {
            this.elements = br.ReadUInt32();
            this.unused = br.ReadUInt32();
            this.offsetTimestamps = br.ReadInt64();
            this.offsetData = br.ReadInt64();

            long save = br.BaseStream.Position;
            br.BaseStream.Position = startOffset + this.offsetTimestamps;

            this.timestamps = new ushort[this.elements];
            this.animationIDs = new ushort[this.elements];
            this.data = new T[this.elements];

            for (uint i = 0; i < this.elements; i++)
            {
                this.timestamps[i] = br.ReadUInt16();
                this.animationIDs[i] = br.ReadUInt16();
            }

            br.BaseStream.Position = startOffset + this.offsetData;
            return save;
        }
    }
}