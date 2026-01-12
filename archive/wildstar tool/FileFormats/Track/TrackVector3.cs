using MathUtils;

namespace ProjectWS.FileFormats.Common
{
    public class TrackVector3 : Track<Vector3>
    {
        public byte value;

        public TrackVector3(BinaryReader br, long startOffset)
        {
            long save = ReadTrackCommon(br, startOffset);

            // Read actual data
            for (uint i = 0; i < this.elements; i++)
            {
                data[i] = new Vector3(br.ReadSingle(), br.ReadSingle(), br.ReadSingle());
            }

            br.BaseStream.Position = save;
        }
    }
}