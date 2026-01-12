using MathUtils;

namespace ProjectWS.FileFormats.Sky
{
    public class TimeTrackVector4 : TimeTrack<Vector4>
    {
        public TimeTrackVector4(BinaryReader br, long startOffset)
        {
            long save = ReadTimeTrackCommon(br, startOffset);

            if (this.data != null)
            {
                // Read actual data
                for (uint i = 0; i < this.elements; i++)
                {
                    this.data[i] = new Vector4(br.ReadSingle(), br.ReadSingle(), br.ReadSingle(), br.ReadSingle());
                }
            }

            br.BaseStream.Position = save;
        }
    }
}