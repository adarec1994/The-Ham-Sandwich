using MathUtils;

namespace ProjectWS.FileFormats.Common
{
    public class TrackQuaternionS : Track<Quaternion>
    {
        public byte value;

        public TrackQuaternionS(BinaryReader br, long startOffset)
        {
            long save = ReadTrackCommon(br, startOffset);

            // Read actual data
            for (uint i = 0; i < this.elements; i++)
            {
                data[i] = new Quaternion();

                short x = br.ReadInt16();
                short y = br.ReadInt16();
                short z = br.ReadInt16();
                short w = br.ReadInt16();

                // TODO : figure out how the quaternion is calculated from the shorts
            }

            br.BaseStream.Position = save;
        }
    }
}