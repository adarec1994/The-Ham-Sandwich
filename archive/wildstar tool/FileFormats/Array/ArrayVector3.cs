using MathUtils;

namespace ProjectWS.FileFormats.Common
{
    public class ArrayVector3 : Array<Vector3>
    {
        public ArrayVector3(BinaryReader br, long startOffset)
        {
            long save = ReadArrayCommon(br, startOffset);

            // Read actual data
            for (uint i = 0; i < this.elements; i++)
            {
                this.data[i] = new Vector3(br.ReadSingle(), br.ReadSingle(), br.ReadSingle());
            }

            br.BaseStream.Position = save;
        }
    }
}