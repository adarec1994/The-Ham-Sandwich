namespace ProjectWS.FileFormats.Sky
{
    public class TimeTrackAngleAndColorAB : TimeTrack<AngleAndColorAB>
    {
        public TimeTrackAngleAndColorAB(BinaryReader br, long startOffset)
        {
            long save = ReadTimeTrackCommon(br, startOffset);

            if (this.data != null)
            {
                // Read actual data
                for (uint i = 0; i < this.elements; i++)
                {
                    this.data[i] = new AngleAndColorAB(br);
                }
            }

            br.BaseStream.Position = save;
        }
    }
}