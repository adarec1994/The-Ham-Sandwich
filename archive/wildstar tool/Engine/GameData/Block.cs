using ProjectWS.FileFormats.Extensions;
using System.Collections;
using System.Collections.Generic;
using System.IO;
using System.Reflection.PortableExecutable;
using static ProjectWS.Engine.Data.Block;

namespace ProjectWS.Engine.Data
{
    public class Block
    {
        public uint directoryCount;
        public uint fileCount;
        public DirectoryEntry[]? directoryEntries;
        public FileEntry[]? fileEntries;

        public Block(BinaryReader br, uint blockIndex, string breadCrumb, Archive archive)
        {
            long save = br.BaseStream.Position;

            br.BaseStream.Position = (long)archive.index.blockHeaders[blockIndex].blockOffset;

            this.directoryCount = br.ReadUInt32();
            this.fileCount = br.ReadUInt32();

            long pos = br.BaseStream.Position;
            long end = pos + (8 * this.directoryCount) + (56 * this.fileCount);

            if (this.directoryCount > 0)
            {
                this.directoryEntries = new DirectoryEntry[this.directoryCount];

                for (int i = 0; i < this.directoryCount; i++)
                {
                    this.directoryEntries[i] = new DirectoryEntry(br, end);
                    
                    string newPath = $"{breadCrumb}\\{this.directoryEntries[i].name}";
                    Block block = new Block(br, this.directoryEntries[i].nextBlock, newPath, archive);
                    archive.blockTree.Add(newPath, block);
                }
            }

            if (this.fileCount > 0)
            {
                this.fileEntries = new FileEntry[this.fileCount];

                for (int i = 0; i < this.fileCount; i++)
                {
                    this.fileEntries[i] = new FileEntry(br, end);

                    var filePath = $"{breadCrumb}\\{this.fileEntries[i].name}";
                    archive.fileList.Add(filePath, this.fileEntries[i]);
                }
            }

            br.BaseStream.Position = save;
        }

        public class DirectoryEntry
        {
            public uint nameOffset;
            public uint nextBlock;
            public string name;

            public DirectoryEntry(BinaryReader br, long dirEnd)
            {
                this.nameOffset = br.ReadUInt32();
                this.nextBlock = br.ReadUInt32();

                long save = br.BaseStream.Position;

                br.BaseStream.Position = dirEnd + this.nameOffset;
                this.name = br.ReadNullTerminatedString();
                br.BaseStream.Position = save;
            }
        }

        public class FileEntry
        {
            public uint nameOffset;
            public Compression compression;
            public ulong writeTime; // uint64 // FILETIME
            public ulong uncompressedSize;
            public ulong compressedSize;
            public byte[] hash;
            public uint unk2;
            public string name;

            public FileEntry(BinaryReader br, long fileEnd)
            {
                this.nameOffset = br.ReadUInt32();
                this.compression = (Compression)br.ReadUInt32();
                this.writeTime = br.ReadUInt64();
                this.uncompressedSize = br.ReadUInt64();
                this.compressedSize = br.ReadUInt64();
                this.hash = br.ReadBytes(20);
                this.unk2 = br.ReadUInt32();

                long save = br.BaseStream.Position;
                br.BaseStream.Position = fileEnd + this.nameOffset;
                this.name = br.ReadNullTerminatedString();
                br.BaseStream.Position = save;
            }

            public enum Compression
            {
                Uncompressed = 1,
                ZLib = 3,
                LZMA = 5,
            }
        }
    }
}