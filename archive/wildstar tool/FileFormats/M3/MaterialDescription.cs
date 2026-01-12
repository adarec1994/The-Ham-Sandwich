using MathUtils;
using ProjectWS.FileFormats.Common;

namespace ProjectWS.FileFormats.M3
{
    public class MaterialDescription : ArrayData
    {
        public short textureSelectorA;
        public short textureSelectorB;
        public int unk0;
        public int unk1;
        public int unk2;
        public int unk3;
        public int unk4;
        public Track<float>? unkTrack018;
        public Track<float>? unkTrack030;
        public Track<float>? unkTrack048;
        public Track<float>? unkTrack060;
        public Track<float>? unkTrack078;
        //public Track<float> unkTrack090;
        public Track<float>? unkTrack0A8;
        public Track<float>? unkTrack0C0;
        public Track<float>? unkTrack0D8;
        public Track<float>? unkTrack0F0;
        public Track<Vector3>? unkTrack108;
        public int unk5;
        public int unk6;

        public override void Read(BinaryReader br, long endOffset)
        {
            this.textureSelectorA = br.ReadInt16();
            this.textureSelectorB = br.ReadInt16();
            this.unk0 = br.ReadInt32();
            this.unk1 = br.ReadInt32();
            this.unk2 = br.ReadInt32();
            this.unk3 = br.ReadInt32();
            this.unk4 = br.ReadInt32();
            this.unkTrack018 = new TrackFloat(br, endOffset);
            this.unkTrack030 = new TrackFloat(br, endOffset);
            this.unkTrack048 = new TrackFloat(br, endOffset);
            this.unkTrack060 = new TrackFloat(br, endOffset);
            this.unkTrack078 = new TrackFloat(br, endOffset);
            //this.unkTrack090 = new TrackFloat(br, endOffset);
            br.BaseStream.Position += 24;       // Don't think this track is in right spot
            this.unkTrack0A8 = new TrackFloat(br, endOffset);
            this.unkTrack0C0 = new TrackFloat(br, endOffset);
            this.unkTrack0D8 = new TrackFloat(br, endOffset);
            this.unkTrack0F0 = new TrackFloat(br, endOffset);
            this.unkTrack108 = new TrackVector3(br, endOffset);
            this.unk5 = br.ReadInt32();
            this.unk6 = br.ReadInt32();
        }

        public override int GetSize()
        {
            return 296;
        }
    }
}