namespace ProjectWS.FileFormats.Area
{
    public partial class SubArea
    {
        public class SkyCorner
        {
            public uint[]? worldSkyIDs;
            public byte[]? worldSkyWeights;

            public SkyCorner(uint[] skyIDs, byte[] skyWeights)
            {
                this.worldSkyIDs = skyIDs;
                this.worldSkyWeights = skyWeights;
            }

            public SkyCorner(BinaryReader br)
            {
                this.worldSkyIDs = new uint[4] { br.ReadUInt32(), br.ReadUInt32(), br.ReadUInt32(), br.ReadUInt32() };
            }

            public void ReadWeights(BinaryReader br)
            {
                this.worldSkyWeights = br.ReadBytes(4);
            }
        }
    }
}
