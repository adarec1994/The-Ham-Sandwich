namespace ProjectWS.FileFormats.Common
{
    public class ArrayString : Array<string>
    {
        public ArrayString(BinaryReader br, long startOffset)
        {
            long save = ReadArrayCommon(br, startOffset);

            // Read actual data
            var save0 = br.BaseStream.Position;
            for (uint i = 0; i < this.elements; i++)
            {
                data[i] = new string(new ArrayWChar(br, save0 + (this.elements * 16)).data);
            }

            br.BaseStream.Position = save;
        }
    }
}