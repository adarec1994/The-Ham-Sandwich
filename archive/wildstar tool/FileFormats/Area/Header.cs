namespace ProjectWS.FileFormats.Area
{
    public struct Header
    {
        public uint magic;
        public uint version;

        public Header(BinaryReader br)
        {
            this.magic = br.ReadUInt32();
            this.version = br.ReadUInt32();
        }

        public Header(uint magic, uint version)
        {
            this.magic = magic;
            this.version = version;
        }

        public void Write(BinaryWriter bw)
        {
            bw.Write(this.magic);
            bw.Write(this.version);
        }
    }
}
