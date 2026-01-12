using ProjectWS.FileFormats.Extensions;

namespace ProjectWS.FileFormats.Common
{
    public class ArrayStruct<T> where T : ArrayData, new()
    {
        public uint elements;
        public uint unused;
        public long offset;
        public T[] data;

        public ArrayStruct(BinaryReader br, long startOffset, bool i3Alignment = false)
        {
            this.elements = br.ReadUInt32();
            this.unused = br.ReadUInt32();
            this.offset = br.ReadInt64();

            long save = br.BaseStream.Position;
            br.BaseStream.Position = startOffset + this.offset;

            long endOffset = 0;
            this.data = new T[this.elements];
            for (uint i = 0; i < this.elements; i++)
            {
                T rec = new T();

                if (endOffset == 0)
                {
                    if (i3Alignment)
                    {
                        endOffset = startOffset + this.offset + (long)((ulong)((rec.GetSize() * this.elements) + 15) & 0xFFFFFFFFFFFFFFF0);
                    }
                    else
                    {
                        endOffset = startOffset + this.offset + (rec.GetSize() * this.elements);
                    }
                }

                rec.Read(br, endOffset);
                data[i] = rec;
            }

            br.BaseStream.Position = save;
        }
    }
}