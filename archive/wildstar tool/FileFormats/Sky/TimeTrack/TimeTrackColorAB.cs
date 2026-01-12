namespace ProjectWS.FileFormats.Sky
{
    public class TimeTrackColorAB : TimeTrack<ColorAB>
    {
        public TimeTrackColorAB(BinaryReader br, long startOffset)
        {
            long save = ReadTimeTrackCommon(br, startOffset);

            if (this.data != null)
            {
                // Read actual data
                for (uint i = 0; i < this.elements; i++)
                {
                    this.data[i] = new ColorAB(br);
                }
            }

            br.BaseStream.Position = save;
        }
    }
}