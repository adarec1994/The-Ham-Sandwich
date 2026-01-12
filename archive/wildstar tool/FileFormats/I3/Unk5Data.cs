using MathUtils;
using ProjectWS.FileFormats.Common;


namespace ProjectWS.FileFormats.I3
{
    public class Unk5Data : ArrayData
    {
        // UNUSED //

        public override int GetSize()
        {
            return 32;
        }

        public override void Read(BinaryReader br, long endOffset = 0)
        {

        }
    }
}
