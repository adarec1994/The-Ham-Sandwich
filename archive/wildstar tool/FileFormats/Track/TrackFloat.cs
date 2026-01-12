namespace ProjectWS.FileFormats.Common
{
    public class TrackFloat : Track<float>
    {
        public byte value;

        public TrackFloat(BinaryReader br, long startOffset)
        {
            long save = ReadTrackCommon(br, startOffset);

            // Read actual data
            for (uint i = 0; i < this.elements; i++)
            {
                data[i] = br.ReadSingle();
            }

            br.BaseStream.Position = save;
        }
    }
}