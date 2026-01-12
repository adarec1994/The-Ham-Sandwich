namespace ProjectWS.FileFormats.Sky
{
    public class TimeTrackAngleAndColor : TimeTrack<AngleAndColor>
    {
        public TimeTrackAngleAndColor(BinaryReader br, long startOffset)
        {
            long save = ReadTimeTrackCommon(br, startOffset);

            if (this.data != null)
            {
                // Read actual data
                for (uint i = 0; i < this.elements; i++)
                {
                    this.data[i] = new AngleAndColor(br);
                }
            }

            br.BaseStream.Position = save;
        }
    }
}