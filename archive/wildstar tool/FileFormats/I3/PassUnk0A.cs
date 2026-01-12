using MathUtils;
using ProjectWS.FileFormats.Common;


namespace ProjectWS.FileFormats.I3
{
    public class PassUnk0A : ArrayData
    {
        public ushort colorTexUnitIndex;
        public ushort normalTexUnitIndex;
        public uint unk0;
        public uint unk1;
        public uint unk2;
        public uint unk3;
        public uint unk4;
        public Track<float>? unkTrack0;
        public Track<float>? unkTrack1;
        public Track<float>? unkTrack2;
        public Track<float>? unkTrack3;
        public uint unk5;
        public uint unk6;
        public uint unk7;
        public uint unk8;
        public Track<float>? unkTrack4;
        public Track<float>? unkTrack5;
        public Track<float>? unkTrack6;
        public Track<float>? unkTrack7;
        public Track<float>? unkTrack8;
        public uint unk9;
        public uint unk10;

        public override int GetSize()
        {
            return 296;
        }

        public override void Read(BinaryReader br, long endOffset = 0)
        {
            this.colorTexUnitIndex = br.ReadUInt16();
            this.normalTexUnitIndex = br.ReadUInt16();
            this.unk0 = br.ReadUInt32();
            this.unk1 = br.ReadUInt32();
            this.unk2 = br.ReadUInt32();
            this.unk3 = br.ReadUInt32();
            this.unk4 = br.ReadUInt32();
            br.BaseStream.Position += 24;       // Unused track
            this.unkTrack0 = new TrackFloat(br, endOffset);
            this.unkTrack1 = new TrackFloat(br, endOffset);
            this.unkTrack2 = new TrackFloat(br, endOffset);
            this.unkTrack3 = new TrackFloat(br, endOffset);
            br.BaseStream.Position += 4;
            this.unk5 = br.ReadUInt32();
            this.unk6 = br.ReadUInt32();
            this.unk7 = br.ReadUInt32();
            this.unk8 = br.ReadUInt32();
            br.BaseStream.Position += 4;
            this.unkTrack4 = new TrackFloat(br, endOffset);
            this.unkTrack5 = new TrackFloat(br, endOffset);
            this.unkTrack6 = new TrackFloat(br, endOffset);
            this.unkTrack7 = new TrackFloat(br, endOffset);
            this.unkTrack8 = new TrackFloat(br, endOffset);
            this.unk9 = br.ReadUInt32();
            this.unk10 = br.ReadUInt32();
        }
    }
}
