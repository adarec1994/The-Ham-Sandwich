namespace ProjectWS.FileFormats.Sky
{
    public class TimeTrackInt : TimeTrack<int>
    {
        public TimeTrackInt(BinaryReader br, long startOffset)
        {
            long save = ReadTimeTrackCommon(br, startOffset);

            if (this.data != null)
            {
                // Read actual data
                for (uint i = 0; i < this.elements; i++)
                {
                    this.data[i] = br.ReadInt32();
                }
            }

            br.BaseStream.Position = save;
        }
    }
}