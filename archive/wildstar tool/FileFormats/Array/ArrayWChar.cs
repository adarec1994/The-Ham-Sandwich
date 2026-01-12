namespace ProjectWS.FileFormats.Common
{
    public class ArrayWChar : Array<Char>
    {
        public ArrayWChar(BinaryReader br, long startOffset)
        {
            long save = ReadArrayCommon(br, startOffset);

            // Read actual data
            for (uint i = 0; i < this.elements; i++)
            {
                this.data[i] = BitConverter.ToChar(br.ReadBytes(2), 0);
            }

            br.BaseStream.Position = save;
        }
    }
}