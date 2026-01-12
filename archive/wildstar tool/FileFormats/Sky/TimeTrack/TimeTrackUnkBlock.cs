namespace ProjectWS.FileFormats.Sky
{
    public class TimeTrackUnkBlock : TimeTrack<SHCoefficients>
    {
        public TimeTrackUnkBlock(BinaryReader br, long startOffset)
        {
            long save = ReadTimeTrackCommon(br, startOffset);

            if (this.data != null)
            {
                // Read actual data
                for (uint i = 0; i < this.elements; i++)
                {
                    this.data[i] = new SHCoefficients(br);
                }
            }

            br.BaseStream.Position = save;
        }
    }
}