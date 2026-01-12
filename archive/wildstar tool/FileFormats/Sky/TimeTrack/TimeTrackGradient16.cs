namespace ProjectWS.FileFormats.Sky
{
    public class TimeTrackGradient16 : TimeTrack<Gradient16>
    {
        public TimeTrackGradient16(BinaryReader br, long startOffset)
        {
            long save = ReadTimeTrackCommon(br, startOffset);

            if (this.data != null)
            {
                // Read actual data
                for (uint i = 0; i < this.elements; i++)
                {
                    this.data[i] = new Gradient16(br);
                }
            }

            br.BaseStream.Position = save;
        }
    }
}