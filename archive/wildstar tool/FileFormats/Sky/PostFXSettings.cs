using MathUtils;
using ProjectWS.FileFormats.Extensions;

namespace ProjectWS.FileFormats.Sky
{
    public class PostFXSettings
    {
        public Vector4 overlayColor;
        public int unk0;
        public int unk1;
        public float finalImageSaturation;         // 0-1; default 1
        public float inverseColorOverlay;          // 0-1; 1 = no overlay, 0 = full overlay; default 1
        public float brightness;                   // 0-X; default 1; (or gamma)
        public float inverseExposure;              // 0-X; default 1; doesn't affect emissives/specular
        public int unk2;
        public int unk3;
        public int unk4;
        public int unk5;
        public int unk6;
        public float inverseGamma;                 // Probably, default 0.109375, can go below 0
        public float bloomAlpha;                   // Overall bloom contribution
        public float bloomStrength;
        public float unk7;                         // 1.0
        public float unk8;                         // 10.0
        public float unk9;                         // 10.0
        public float unk10;                         // 1.0
        public int unk11;
        public int unk12;

        public PostFXSettings(BinaryReader br)
        {
            this.overlayColor = br.ReadColor();
            this.unk0 = br.ReadInt32();
            this.unk1 = br.ReadInt32();
            this.finalImageSaturation = br.ReadSingle();
            this.inverseColorOverlay = br.ReadSingle();
            this.brightness = br.ReadSingle();
            this.inverseExposure = br.ReadSingle();
            this.unk2 = br.ReadInt32();
            this.unk3 = br.ReadInt32();
            this.unk4 = br.ReadInt32();
            this.unk5 = br.ReadInt32();
            this.unk6 = br.ReadInt32();
            this.inverseGamma = br.ReadSingle();
            this.bloomAlpha = br.ReadSingle();
            this.bloomStrength = br.ReadSingle();
            this.unk7 = br.ReadSingle();
            this.unk8 = br.ReadSingle();
            this.unk9 = br.ReadSingle();
            this.unk10 = br.ReadSingle();
            this.unk11 = br.ReadInt32();
            this.unk12 = br.ReadInt32();
        }
    }

}
