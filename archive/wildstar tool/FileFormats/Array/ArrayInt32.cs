namespace ProjectWS.FileFormats.Common
{
    public class ArrayInt32 : Array<int>
    {
        public ArrayInt32(BinaryReader br, long startOffset)
        {
            long save = ReadArrayCommon(br, startOffset);

            // Read actual data
            for (uint i = 0; i < this.elements; i++)
            {
                data[i] = br.ReadInt32();
            }

            br.BaseStream.Position = save;
        }
    }
}