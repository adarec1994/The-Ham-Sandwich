using MathUtils;
using ProjectWS.FileFormats.Common;

namespace ProjectWS.FileFormats.Sky
{
    public partial class File
    {
        public string filePath;
        public bool failedReading;

        const int headerSize = 1216;

        public uint version;
        public int unk0;            // 0,1
        public int unk1;            // 0,1
        public float unk2;
        public string sourceFilePath;
        public SHGroup skyDataBlock0;
        public SHGroup skyDataBlock1;
        public SHGroup skyDataBlock2;
        public SHGroup skyDataBlock3;
        public TimeTrack<Vector4> sunLightColor;
        public TimeTrack<AngleAndColor> specularColor;
        public TimeTrack<Vector4> unkColorData;
        public TimeTrack<FogSettings> fogSettings;
        public SkyboxModel[] skyboxModels;
        public TimeTrack<PostFXSettings> postFXSettings;
        public TimeTrack<int> unk9;
        public TimeTrack<int> unk10;
        public string[] particulates;
        public string environmentMapPath;
        public TimeTrack<Vector4> unk13;
        public string sunGlarePathA;
        public string sunGlarePathB;
        public TimeTrack<float> unk16;
        public TimeTrack<Vector4> unk17;
        public TimeTrack<Vector4> unk18;
        public TimeTrack<ColorAB> unk19;
        public TimeTrack<ColorAB> unk20;
        public TimeTrack<ColorAB> unk21;
        public TimeTrack<ColorAB> unk22;
        public TimeTrack<ColorAB> unk23;
        public TimeTrack<ColorAB> unk24;
        public TimeTrack<ColorAB> unk25;
        public TimeTrack<ColorAB> unk26;
        public UnkData[] unkData;
        public string lutFile;

        public File(string filePath)
        {
            this.filePath = filePath;
            this.failedReading = false;
        }

        public void Read(Stream str)
        {
            try
            {
                if (str == null)
                {
                    this.failedReading = true;
                    return;
                }

                using (BinaryReader br = new BinaryReader(str))
                {
                    int magic = br.ReadInt32();
                    if (magic != 0x58534B59)    // XSKY
                    {
                        this.failedReading = true;
                        Console.WriteLine($"This is not an SKY file, 0x{magic:X} != XSKY, {this.filePath}");
                        return;
                    }

                    this.version = br.ReadUInt32();
                    this.unk0 = br.ReadInt32();
                    this.unk1 = br.ReadInt32();
                    this.unk2 = br.ReadSingle();
                    br.BaseStream.Position += 4;    // Padding
                    this.sourceFilePath = new string(new ArrayWChar(br, headerSize).data);
                    this.skyDataBlock0 = new SHGroup(br, headerSize);
                    this.skyDataBlock1 = new SHGroup(br, headerSize);
                    this.skyDataBlock2 = new SHGroup(br, headerSize);
                    this.skyDataBlock3 = new SHGroup(br, headerSize);
                    this.sunLightColor = new TimeTrackVector4(br, headerSize);
                    this.specularColor = new TimeTrackAngleAndColor(br, headerSize);
                    this.unkColorData = new TimeTrackVector4(br, headerSize);
                    this.fogSettings = new TimeTrackFogSettings(br, headerSize);
                    this.skyboxModels = new ArrayStruct<SkyboxModel>(br, headerSize).data;
                    this.postFXSettings = new TimeTrackPostFXSettings(br, headerSize);
                    this.unk9 = new TimeTrackInt(br, headerSize);
                    this.unk10 = new TimeTrackInt(br, headerSize);
                    this.particulates = new ArrayString(br, headerSize).data;
                    this.environmentMapPath = new string(new ArrayWChar(br, headerSize).data);
                    this.unk13 = new TimeTrackVector4(br, headerSize);
                    this.sunGlarePathA = new string(new ArrayWChar(br, headerSize).data);
                    this.sunGlarePathB = new string(new ArrayWChar(br, headerSize).data);
                    this.unk16 = new TimeTrackFloat(br, headerSize);
                    this.unk17 = new TimeTrackVector4(br, headerSize);
                    this.unk18 = new TimeTrackVector4(br, headerSize);
                    this.unk19 = new TimeTrackColorAB(br, headerSize);
                    this.unk20 = new TimeTrackColorAB(br, headerSize);
                    this.unk21 = new TimeTrackColorAB(br, headerSize);
                    this.unk22 = new TimeTrackColorAB(br, headerSize);
                    this.unk23 = new TimeTrackColorAB(br, headerSize);
                    this.unk24 = new TimeTrackColorAB(br, headerSize);
                    this.unk25 = new TimeTrackColorAB(br, headerSize);
                    this.unk26 = new TimeTrackColorAB(br, headerSize);
                    this.unkData = new ArrayStruct<UnkData>(br, headerSize).data;
                    this.lutFile = new string(new ArrayWChar(br, headerSize).data);
                }
            }
            catch (Exception e)
            {
                this.failedReading = true;
                Console.WriteLine($"SKY : Failed Reading File {this.filePath}");
                Console.WriteLine(e.Message);
            }
        }
    }
}