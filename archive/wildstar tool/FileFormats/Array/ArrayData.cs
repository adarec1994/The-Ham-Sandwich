namespace ProjectWS.FileFormats.Common
{
    public abstract class ArrayData
    {

        public abstract void Read(BinaryReader br, long endOffset = 0);
        public abstract int GetSize();
    }
}