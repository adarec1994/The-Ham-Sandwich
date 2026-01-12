namespace ProjectWS.FileFormats.Sky
{
    public class FogSettings
    {
        public float unk0;
        public float fogStartDistance;
        public float unk1;
        public float unk2;
        public float unk3;

        public FogSettings(BinaryReader br)
        {
            this.unk0 = br.ReadSingle();
            this.fogStartDistance = br.ReadSingle();
            this.unk1 = br.ReadSingle();
            br.BaseStream.Position += 4;        // Gap
            this.unk2 = br.ReadSingle();
            this.unk3 = br.ReadSingle();
        }

        public override string ToString()
        {
            return $"FogSettings({this.unk0} {this.fogStartDistance} {this.unk1} {this.unk2} {this.unk3})";
        }
    }
}
