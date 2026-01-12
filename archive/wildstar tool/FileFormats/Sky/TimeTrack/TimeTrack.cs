using ProjectWS.FileFormats.Common;
using ProjectWS.FileFormats.Extensions;

namespace ProjectWS.FileFormats.Sky
{
    public abstract class TimeTrack<T> : ArrayData
    {
        public uint elements;
        public long offsetTimestamps;
        public long offsetData;

        public uint[]? timestamps;
        public T[]? data;

        public long ReadTimeTrackCommon(BinaryReader br, long startOffset)
        {
            this.elements = br.ReadUInt32();
            br.BaseStream.Position += 4;    // Gap
            this.offsetTimestamps = br.ReadInt64();
            this.offsetData = br.ReadInt64();

            long save = br.BaseStream.Position;
            br.BaseStream.Position = startOffset + this.offsetTimestamps;
            br.Align(16);

            this.timestamps = new uint[this.elements];
            this.data = new T[this.elements];

            for (uint i = 0; i < this.elements; i++)
            {
                this.timestamps[i] = br.ReadUInt32();
            }

            br.BaseStream.Position = startOffset + this.offsetData;
            br.Align(16);
            return save;
        }

        public override void Read(BinaryReader br, long endOffset = 0)
        {
            return;
        }

        public override int GetSize()
        {
            return 0;
        }
    }
}