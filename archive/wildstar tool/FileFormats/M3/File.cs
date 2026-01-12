using ProjectWS.FileFormats.Common;

namespace ProjectWS.FileFormats.M3
{
    public class File
    {
        public string filePath;
        public string fileName;
        public bool failedReading;
        public int modelID = 1;

        public int headerSize;
        public uint version;

        public Bounds[] bounds;
        public Track<byte> unk020;
        public Track<byte> unk038;
        public Track<byte> unk050;
        public Track<byte> unk068;
        public Unk080[] unk080;
        public Track<byte> unk090;
        public Track<byte> unk0A8;
        public Track<byte> unk0C0;
        public Track<byte> unk0D8;
        public Unk0F0[] unk0F0;
        public Track<uint> unk100;
        public Track<uint> unk118;
        public Track<byte> unk130;
        public Track<byte> unk148;
        public Track<uint> unk160;
        public Bone[] bones;
        public ushort[] unk190;
        public ushort[] unk1A0;
        public short[] boneMapping;
        public Texture[] textures;
        public short[] unk1D0;          // Material IDs ?
        public Unk1E0[] unk1E0;
        public Material[] materials;
        public Model2Display[] model2Displays;
        public short[] display2Models;
        public Unk220[] unk220;
        public Unk230[] unk230;
        public Unk240[] unk240;
        public Geometry[] geometries;

        public File(string filePath)
        {
            this.filePath = filePath;
            this.fileName = Path.GetFileNameWithoutExtension(filePath);
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

                if (str.Length <= 0)
                {
                    this.failedReading = true;
                    return;
                }

                using (BinaryReader br = new BinaryReader(str))
                {
                    int magic = br.ReadInt32();
                    if (magic != 0x4D4F444C)    // MODL
                    {
                        this.failedReading = true;
                        Console.WriteLine($"This is not an M3 file, 0x{magic:X} != MODL, {this.filePath}");
                        return;
                    }

                    this.version = br.ReadUInt32();

                    if (this.version != 100)
                    {
                        this.failedReading = true;
                        Console.WriteLine($"Unsupported M3 version {this.version}");
                        return;
                    }

                    if (this.version == 100)
                    {
                        this.headerSize = 1584;
                    }

                    br.BaseStream.Position += 8;        // Padding
                    this.bounds = new ArrayStruct<Bounds>(br, this.headerSize).data;
                    this.unk020 = new TrackUint8(br, this.headerSize);
                    this.unk038 = new TrackUint8(br, this.headerSize);
                    this.unk050 = new TrackUint8(br, this.headerSize);
                    this.unk068 = new TrackUint8(br, this.headerSize);
                    this.unk080 = new ArrayStruct<Unk080>(br, this.headerSize).data;
                    this.unk090 = new TrackUint8(br, this.headerSize);
                    this.unk0A8 = new TrackUint8(br, this.headerSize);
                    this.unk0C0 = new TrackUint8(br, this.headerSize);
                    this.unk0D8 = new TrackUint8(br, this.headerSize);
                    this.unk0F0 = new ArrayStruct<Unk0F0>(br, this.headerSize).data;
                    this.unk100 = new TrackUint32(br, this.headerSize);
                    this.unk118 = new TrackUint32(br, this.headerSize);
                    this.unk130 = new TrackUint8 (br, this.headerSize);
                    this.unk148 = new TrackUint8(br, this.headerSize);
                    this.unk160 = new TrackUint32(br, this.headerSize);
                    br.BaseStream.Position += 8;        // Padding
                    this.bones = new ArrayStruct<Bone>(br, this.headerSize).data;
                    this.unk190 = new ArrayUInt16(br, this.headerSize).data;
                    this.unk1A0 = new ArrayUInt16(br, this.headerSize).data;
                    this.boneMapping = new ArrayInt16(br, this.headerSize).data;
                    this.textures = new ArrayStruct<Texture>(br, this.headerSize).data;
                    this.unk1D0 = new ArrayInt16(br, this.headerSize).data;
                    this.unk1E0 = new ArrayStruct<Unk1E0>(br, this.headerSize).data;
                    this.materials = new ArrayStruct<Material>(br, this.headerSize).data;
                    this.model2Displays = new ArrayStruct<Model2Display>(br, this.headerSize).data;
                    this.display2Models = new ArrayInt16(br, this.headerSize).data;
                    this.unk220 = new ArrayStruct<Unk220>(br, this.headerSize).data;
                    this.unk230 = new ArrayStruct<Unk230>(br, this.headerSize).data;
                    this.unk240 = new ArrayStruct<Unk240>(br, this.headerSize).data;
                    this.geometries = new ArrayStruct<Geometry>(br, this.headerSize).data;
                    // .. many unks ..
                }
            }
            catch (Exception e)
            {
                this.failedReading = true;
                Console.WriteLine($"M3 : Failed Reading File {this.filePath}");
                Console.WriteLine(e.Message);
            }
        }
    }
}