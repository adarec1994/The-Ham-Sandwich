using ProjectWS.FileFormats.Extensions;

namespace ProjectWS.Engine.Data
{
    public class Archive
    {
        public bool failedReading;
        public string dataPath;
        public string name;
        public Index index;
        public Data data;
        public Dictionary<string, Block> blockTree;
        public Dictionary<string, Block.FileEntry> fileList;
        public const string rootBlockName = "Root";

        public Archive(string dataPath, string name)
        {
            this.dataPath = dataPath;
            this.name = name;
            this.index = new Index(this);
            this.data = new Data(this);
            this.blockTree = new Dictionary<string, Block>();
            this.fileList = new Dictionary<string, Block.FileEntry>();
        }

        public void Read(Action<int>? progress = null)
        {
            if (!File.Exists(this.index.filePath))
            {
                this.failedReading = true;
                Debug.LogWarning($"Couldn't find index file at path : {this.index.filePath}");
                return;
            }

            if (!File.Exists(this.data.filePath))
            {
                this.failedReading = true;
                Debug.LogWarning($"Couldn't find archive file at path : {this.data.filePath}");
                return;
            }

            this.index.Read();
            progress?.Invoke(0);
            this.data.Read(progress);
        }

        public struct Header
        {
            public string magic;    // PACK
            public uint version;
            public ulong fileSize;
            public ulong offsetBlockTable;
            public uint numberOfBlocks;

            public Header(BinaryReader br)
            {
                this.magic = br.ReadChunkID();
                this.version = br.ReadUInt32();
                br.BaseStream.Position += 512;      // Padding ?
                this.fileSize = br.ReadUInt64();
                br.BaseStream.Position += 8;        // Padding ?
                this.offsetBlockTable = br.ReadUInt64();
                this.numberOfBlocks = br.ReadUInt32();
                br.BaseStream.Position += 28;       // Skipping unknown
            }
        }

        public struct BlockHeader
        {
            public ulong blockOffset;
            public ulong blockSize;

            public BlockHeader(BinaryReader br)
            {
                this.blockOffset = br.ReadUInt64();
                this.blockSize = br.ReadUInt64();
            }
        }

        public class Index
        {
            Archive archive;
            public string filePath;

            public Header header;
            public BlockHeader[] blockHeaders;
            public int aidxBlockIndex;
            public AIDX aidx;
            public Block rootBlock;

            public Index(Archive archive)
            {
                this.archive = archive;
                this.filePath = $"{archive.dataPath}\\{archive.name}.index";
            }

            public void Read()
            {
                try
                {
                    using (Stream str = File.OpenRead(this.filePath))
                    {
                        using (BinaryReader br = new BinaryReader(str))
                        {
                            // Header
                            this.header = new Header(br);

                            // Block Headers
                            br.BaseStream.Position = (long)this.header.offsetBlockTable;
                            this.blockHeaders = new BlockHeader[this.header.numberOfBlocks];
                            for (int i = 0; i < this.blockHeaders.Length; i++)
                            {
                                this.blockHeaders[i] = new BlockHeader(br);
                                if (this.blockHeaders[i].blockSize == 16)
                                {
                                    this.aidxBlockIndex = i;
                                }
                            }

                            // AIDX Block
                            br.BaseStream.Position = (long)this.blockHeaders[this.aidxBlockIndex].blockOffset;
                            this.aidx = new AIDX(br);

                            // Blocks Tree
                            this.rootBlock = new Block(br, this.aidx.rootBlock, rootBlockName, this.archive);
                            this.archive.blockTree.Add(rootBlockName, this.rootBlock); // Add root
                        }
                    }
                }
                catch(Exception e)
                {
                    Debug.LogException(e);
                    this.archive.failedReading = true;
                    return;
                }
            }

            public struct AIDX
            {
                public string magic;
                public uint version;
                public uint unk0;
                public uint rootBlock;

                public AIDX(BinaryReader br)
                {
                    this.magic = br.ReadChunkID();
                    this.version = br.ReadUInt32();
                    this.unk0 = br.ReadUInt32();
                    this.rootBlock = br.ReadUInt32();
                }
            }
        }

        public class Data
        {
            Archive archive;
            public string filePath;

            public Header header;
            public BlockHeader[] blockHeaders;
            public int aarcBlockIndex;
            public AARC aarc;
            public Dictionary<byte[], Entry> aarcEntries;

            public Data(Archive archive)
            {
                this.archive = archive;
                this.filePath = $"{archive.dataPath}\\{archive.name}.archive";
            }
            
            public void Read(Action<int>? progress = null)
            {
                try
                {
                    using (Stream str = File.OpenRead(this.filePath))
                    {
                        using (BinaryReader br = new BinaryReader(str))
                        {
                            // Header
                            this.header = new Header(br);

                            // Block Headers
                            br.BaseStream.Position = (long)this.header.offsetBlockTable;
                            this.blockHeaders = new BlockHeader[this.header.numberOfBlocks];
                            for (int i = 0; i < this.blockHeaders.Length; i++)
                            {
                                this.blockHeaders[i] = new BlockHeader(br);
                                if (this.blockHeaders[i].blockSize == 16)
                                {
                                    this.aarcBlockIndex = i;
                                }
                            }

                            progress?.Invoke(0);

                            // AARC Block
                            br.BaseStream.Position = (long)this.blockHeaders[this.aarcBlockIndex].blockOffset;
                            this.aarc = new AARC(br);

                            // AARC Entries
                            br.BaseStream.Position = (long)this.blockHeaders[(int)this.aarc.entriesOffset].blockOffset;
                            this.aarcEntries = new Dictionary<byte[], Entry>(new ByteArrayComparer());
                            for (int i = 0; i < this.aarc.entriesCount; i++)
                            {
                                var entry = new Entry(br);
                                if (!this.aarcEntries.ContainsKey(entry.hash))
                                    this.aarcEntries.Add(entry.hash, entry);
                            }

                            progress?.Invoke(0);
                        }
                    }
                }
                catch (Exception e)
                {
                    Debug.LogException(e);
                    this.archive.failedReading = true;
                    return;
                }
            }

            public struct AARC
            {
                public string magic;
                public uint version;
                public uint entriesCount;
                public uint entriesOffset;

                public AARC(BinaryReader br)
                {
                    this.magic = br.ReadChunkID();
                    this.version = br.ReadUInt32();
                    this.entriesCount = br.ReadUInt32();
                    this.entriesOffset = br.ReadUInt32();
                }
            }

            public struct Entry
            {
                public uint blockIndex;
                public byte[] hash;
                public ulong uncompressedSize;

                public Entry(BinaryReader br)
                {
                    this.blockIndex = br.ReadUInt32();
                    this.hash = br.ReadBytes(20);
                    this.uncompressedSize = br.ReadUInt64();
                }
            }
        }
    }
}