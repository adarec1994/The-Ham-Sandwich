namespace ProjectWS.FileFormats.Common
{
    public class TrackUint32 : Track<uint>
    {
        public byte value;

        public TrackUint32(BinaryReader br, long startOffset)
        {
            long save = ReadTrackCommon(br, startOffset);

            // Read actual data
            for (uint i = 0; i < this.elements; i++)
            {
                data[i] = br.ReadUInt32();
            }

            br.BaseStream.Position = save;
        }
    }
}