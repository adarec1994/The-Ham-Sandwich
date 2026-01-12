using MathUtils;

namespace ProjectWS.FileFormats.Common
{
    public class TrackVector3S : Track<Vector3>
    {
        public byte value;

        public TrackVector3S(BinaryReader br, long startOffset)
        {
            long save = ReadTrackCommon(br, startOffset);

            // Read actual data
            for (uint i = 0; i < this.elements; i++)
            {
                data[i] = new Vector3();

                short x = br.ReadInt16();
                short y = br.ReadInt16();
                short z = br.ReadInt16();

                // TODO : figure out how the vector is calculated from the shorts
            }

            br.BaseStream.Position = save;
        }
    }
}