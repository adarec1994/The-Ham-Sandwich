namespace ProjectWS.FileFormats.Sky
{
    public class TimeTrackFloat : TimeTrack<float>
    {
        public TimeTrackFloat(BinaryReader br, long startOffset)
        {
            long save = ReadTimeTrackCommon(br, startOffset);

            if (this.data != null)
            {
                // Read actual data
                for (uint i = 0; i < this.elements; i++)
                {
                    this.data[i] = br.ReadSingle();
                }
            }

            br.BaseStream.Position = save;
        }
    }
}