namespace ProjectWS.FileFormats.Common
{
    public class TrackUint8 : Track<byte>
    {
        public byte value;

        public TrackUint8(BinaryReader br, long startOffset)
        {
            long save = ReadTrackCommon(br, startOffset);

            // Read actual data
            for (uint i = 0; i < this.elements; i++)
            {
                data[i] = br.ReadByte();
            }

            br.BaseStream.Position = save;
        }
    }
}