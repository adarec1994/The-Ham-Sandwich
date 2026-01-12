using ProjectWS.FileFormats.Extensions;
using System.IO;

namespace ProjectWS.FileFormats.Common
{
    public abstract class Array<T>
    {
        public uint elements;
        public uint unused;
        public long offset;

        public T[]? data;

        public long ReadArrayCommon(BinaryReader br, long startOffset)
        {
            this.elements = br.ReadUInt32();
            this.unused = br.ReadUInt32();
            this.offset = br.ReadInt64();

            long save = br.BaseStream.Position;
            this.data = new T[this.elements];
            br.BaseStream.Position = startOffset + this.offset;

            return save;
        }
    }
}