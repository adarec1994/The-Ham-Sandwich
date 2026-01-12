using ProjectWS.FileFormats.Common;

namespace ProjectWS.FileFormats.I3
{
    public class I3File
    {
        const int HEADER_SIZE = 368;

        public string? filePath;
        public string? fileName;
        public bool failedReading;
        public uint version;
        public Sector[]? sectors;
        public Portal[]? portals;
        public Pass[]? passes;
        public short[]? unk1;           // Unused
        public Unk2Data[]? unk2;
        public Unk3Data[]? unk3;
        public Light[]? lights;
        public Unk5Data[]? unk5;        // Unused

        public I3File(string filePath)
        {
            this.filePath = filePath;
            this.fileName = Path.GetFileNameWithoutExtension(filePath);
            this.failedReading = false;
        }

        public void Read(Stream str)
        {
            //try
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
                    if (magic != 0x49444546)    // IDEF I Definition
                    {
                        if (magic == 0x53444546)        // SDEF Sector Definition
                        {
                            this.failedReading = true;
                            return;
                        }

                        this.failedReading = true;
                        Console.WriteLine($"This is not an I3 file, 0x{magic:X} != IDEF, {this.filePath}");
                        return;
                    }

                    this.version = br.ReadUInt32();

                    if (this.version != 41)
                    {
                        this.failedReading = true;
                        Console.WriteLine($"Unsupported I3 version {this.version}");
                        return;
                    }

                    br.BaseStream.Position += 24;
                    this.sectors = new ArrayStruct<Sector>(br, HEADER_SIZE, true).data;
                    this.portals = new ArrayStruct<Portal>(br, HEADER_SIZE, true).data;
                    this.passes = new ArrayStruct<Pass>(br, HEADER_SIZE, true).data;
                    this.unk1 = new ArrayInt16(br, HEADER_SIZE).data;
                    this.unk2 = new ArrayStruct<Unk2Data>(br, HEADER_SIZE, true).data;
                    this.unk3 = new ArrayStruct<Unk3Data>(br, HEADER_SIZE, true).data;
                    this.lights = new ArrayStruct<Light>(br, HEADER_SIZE, true).data;
                    this.unk5 = new ArrayStruct<Unk5Data>(br, HEADER_SIZE, true).data;
                }
            }
            /*
            catch (Exception e)
            {
                this.failedReading = true;
                Console.WriteLine($"I3 : Failed Reading File {this.filePath}");
                Console.WriteLine(e.Message);
            }
            */
        }
    }
}