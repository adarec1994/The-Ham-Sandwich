using ProjectWS.FileFormats.Common;

namespace ProjectWS.FileFormats.Sho
{
    public class File
    {
        public string filePath;
        public bool failedReading;

        public Variant[]? variants;

        public File(string filePath)
        {
            this.filePath = filePath;
            this.failedReading = false;
        }

        public void Read()
        {
            using (Stream str = System.IO.File.OpenRead(this.filePath))
            {
                Read(str);
            }
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
                    uint magic = br.ReadUInt32();
                    uint version = br.ReadUInt32();
                    this.variants = new ArrayStruct<Variant>(br, 80).data;
                    // UnkArray 0
                    // UnkArray 1
                    // UnkArray 2
                    // Unk uint64
                }
            }
            catch (Exception e)
            {
                this.failedReading = true;
                Console.WriteLine($"SHO : Failed Reading File {this.filePath}");
                Console.WriteLine(e.Message);
            }
        }
    }
}
