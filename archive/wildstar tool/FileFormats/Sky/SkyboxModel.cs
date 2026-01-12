using ProjectWS.FileFormats.Common;

namespace ProjectWS.FileFormats.Sky
{
    public class SkyboxModel : ArrayData
    {
        public short unk0;
        public short unk1;
        public int padding;
        public string? filePath;
        public TimeTrack<AngleAndColor>? angleAndColor;

        public override void Read(BinaryReader br, long startOffs)
        {
            this.unk0 = br.ReadInt16();
            this.unk1 = br.ReadInt16();
            this.padding = br.ReadInt32();
            this.filePath = new string(new ArrayWChar(br, startOffs).data);
            this.angleAndColor = new TimeTrackAngleAndColor(br, startOffs);
        }

        public override int GetSize()
        {
            return 48;
        }
    }
}
