using BCnEncoder.Encoder;
using BCnEncoder.Shared;
using MathUtils;
using ProjectWS.FileFormats.Extensions;
using System.IO.Compression;

namespace ProjectWS.FileFormats.Area
{
    public class File
    {
        string? filePath;

        const uint area = 1634887009;
        const uint AREA = 1095910721;

        public Header header;
        public List<SubArea>? subAreas;    // Variable size, not always 16*16
        public List<AreaProp>? props;
        public Dictionary<uint, AreaProp>? propLookup;
        public HashSet<uint>? renderedProps;
        public Curt[]? curts;

        public File(string filePath)
        {
            this.filePath = filePath;
        }

        public void Create()
        {
            this.header = new Header(1634887009, 0);
            this.subAreas = new List<SubArea>();
            int idx = 0;
            for (int i = 0; i < 16 * 16; i++)
            {
                SubArea sub = new SubArea(idx);
                this.subAreas.Add(sub);
                idx++;
            }

            this.props = new List<AreaProp>();
            this.propLookup = new Dictionary<uint, AreaProp>();
            //this.curts = new Curt[0];
            ProcessForExport();
        }

        public void Read(Stream str)
        {
            if (str == null) return;
            if (str.Length == 0) return;

            using (var br = new BinaryReader(str))
            {
                Read(br);
            }
        }

        void Read(BinaryReader br)
        {
            this.header = new Header(br);

            long streamPos = br.BaseStream.Position;

            while (streamPos < br.BaseStream.Length)
            {
                br.BaseStream.Position = streamPos;
                ChunkID chunkID = (ChunkID)br.ReadUInt32();
                int chunkSize = br.ReadInt32();
                streamPos = br.BaseStream.Position + chunkSize;

                switch (chunkID)
                {
                    case ChunkID.CHNK:
                        {
                            if (this.header.magic == AREA)
                            {
                                // Compressed Area file
                                var chunkStart = br.BaseStream.Position;
                                var chunkData = br.ReadBytes(chunkSize);
                                using MemoryStream chunkMS = new MemoryStream(chunkData);
                                using BinaryReader chunkBR = new BinaryReader(chunkMS);
                                int size = chunkBR.ReadInt32();
                                var save = chunkBR.BaseStream.Position;
                                using var headerZlibStream = new ZLibStream(chunkMS, CompressionMode.Decompress);
                                Span<byte> decompressedHeader = new byte[1024];
                                headerZlibStream.Read(decompressedHeader);
                                using MemoryStream decompHeaderMS = new MemoryStream(decompressedHeader.ToArray());
                                using BinaryReader decompHeaderBR = new BinaryReader(decompHeaderMS);
                                List<uint> blobSizes = new List<uint>();
                                List<byte[]> blobs = new List<byte[]>();
                                chunkBR.BaseStream.Position = save + size;
                                for (int i = 0; i < 1024 / 4; i++)
                                {
                                    uint blobSize = decompHeaderBR.ReadUInt32();
                                    blobSizes.Add(blobSize);
                                    if (blobSize != 0)
                                    {
                                        blobs.Add(chunkBR.ReadBytes((int)blobSize));
                                    }
                                    else
                                    {
                                        blobs.Add(null);
                                    }
                                }

                                this.subAreas = new List<SubArea>();

                                for (int i = 0; i < blobs.Count; i++)
                                {
                                    if (blobSizes[i] != 0)
                                    {
                                        using MemoryStream blobMS = new MemoryStream(blobs[i]);
                                        using BinaryReader blobBR = new BinaryReader(blobMS);
                                        var decompSize = blobBR.ReadInt32();
                                        using var blobZlibStream = new ZLibStream(blobMS, CompressionMode.Decompress);

                                        Span<byte> decompData = new byte[decompSize];
                                        blobZlibStream.Read(decompData);

                                        var decompBlob = new byte[decompSize + 4];

                                        // Copy size at the front of the data
                                        var bSize = BitConverter.GetBytes(decompSize);
                                        decompBlob[0] = bSize[0];
                                        decompBlob[1] = bSize[1];
                                        decompBlob[2] = bSize[2];
                                        decompBlob[3] = bSize[3];

                                        // Copy data after size
                                        Array.Copy(decompData.ToArray(), 0, decompBlob, 4, decompSize);

                                        using (var decompDataMS = new MemoryStream(decompBlob))
                                        {
                                            using (var decompDataBR = new BinaryReader(decompDataMS))
                                            {
                                                var subchunk = new SubArea(decompDataBR, i, true);
                                                this.subAreas.Add(subchunk);
                                            }
                                        }
                                    }
                                }

                                br.BaseStream.Position = chunkStart + chunkSize;
                            }

                            if (this.header.magic == area)
                            {
                                this.subAreas = new List<SubArea>();
                                long save = br.BaseStream.Position;
                                int index = 0;

                                while (br.BaseStream.Position < save + chunkSize)
                                {
                                    var subchunk = new SubArea(br, index++, false);
                                    this.subAreas.Add(subchunk);
                                }
                            }
                        }
                        break;
                    case ChunkID.PROp:
                        {
                            long chunkStart = br.BaseStream.Position;
                            int propCount = br.ReadInt32();
                            this.props = new List<AreaProp>();
                            this.propLookup = new Dictionary<uint, AreaProp>();
                            for (int i = 0; i < propCount; i++)
                            {
                                this.props.Add(new AreaProp(br, chunkStart));
                                this.propLookup.Add(this.props[i].uniqueID, this.props[i]);
                            }
                            //File.WriteAllText($"D:/Props_{this.chunk.coords}.json", JsonConvert.SerializeObject(this.props, Formatting.Indented));
                            br.BaseStream.Position = chunkStart + chunkSize;
                        }
                        break;
                    case ChunkID.CURT:
                        {
                            var chunkStart = br.BaseStream.Position;
                            uint curtCount = br.ReadUInt32();
                            this.curts = new Curt[curtCount];
                            for (int i = 0; i < curtCount; i++)
                            {
                                this.curts[i] = new Curt(br, chunkStart);
                            }
                        }
                        break;
                    default:
                        br.SkipChunk(chunkID.ToString(), chunkSize, this.GetType().ToString());
                        break;
                }
            }
        }

        public uint AddProp(string path, Vector3 position, Quaternion rotation, float scale)
        {
            if (this.props == null)
                this.props = new List<AreaProp>();

            if (this.propLookup == null)
                this.propLookup = new Dictionary<uint, AreaProp>();

            AreaProp p = new AreaProp(55419076, path);
            p.position = position;
            p.rotation = rotation;
            p.scale = scale;

            p.color0 = Color.White;// -8421505;
            p.color1 = Color.White;
            p.color2 = Color.White;

            this.props.Add(p);
            this.propLookup.Add(p.uniqueID, p);

            // TODO : Determine which chunks it fits in
            for (int i = 0; i < this.subAreas?.Count; i++)
            {
                if (this.subAreas[i].propUniqueIDs == null)
                    this.subAreas[i].propUniqueIDs = new List<uint>();

                this.subAreas[i].propUniqueIDs.Add(p.uniqueID);
            }


            return p.uniqueID;
        }

        public void ProcessForExport()
        {
            foreach (var sc in subAreas)
            {
                if (sc.blendMap != null)
                {
                    if (sc.blendMap.Length == 65 * 65 * 4 && sc.flags.HasFlag(SubArea.Flags.hasBlendMapDXT))
                    {
                        // Convert chunk to dxt
                        BcEncoder encoder = new BcEncoder();
                        encoder.OutputOptions.GenerateMipMaps = false;
                        encoder.OutputOptions.Quality = CompressionQuality.Balanced;
                        encoder.OutputOptions.Format = CompressionFormat.Bc1;
                        sc.blendMap = encoder.EncodeToRawBytes(sc.blendMap, 65, 65, PixelFormat.Rgba32, 0, out int _, out int _);
                    }
                }
            }
        }

        public void Write()
        {
            if (System.IO.File.Exists(this.filePath))
                System.IO.File.Delete(this.filePath);

            using(var str = System.IO.File.OpenWrite(this.filePath))
            {
                using(BinaryWriter bw = new BinaryWriter(str))
                {
                    this.header.Write(bw);
                    if (this.subAreas != null && this.subAreas.Count > 0)
                    {
                        long chnkStart = bw.BaseStream.Position;
                        bw.Write((uint)1128812107); // CHNK
                        bw.Write((uint)0);          // Size pad
                        long subStart = bw.BaseStream.Position;
                        for (int i = 0; i < this.subAreas.Count; i++)
                        {
                            this.subAreas[i].Write(bw);
                        }
                        long subEnd = bw.BaseStream.Position;
                        uint chnkSize = (uint)(subEnd - subStart);
                        bw.BaseStream.Position = chnkStart + 4;
                        bw.Write(chnkSize);         // Size calculated
                        bw.BaseStream.Position = subEnd;
                    }

                    if (this.props != null && this.props.Count > 0)
                    {
                        long chnkStart = bw.BaseStream.Position;
                        bw.Write((uint)1347571568);     // PROp
                        bw.Write((uint)0);              // Size pad
                        long subStart = bw.BaseStream.Position;
                        bw.Write(this.props.Count);    // Prop count
                        Dictionary<string, uint> paths = new Dictionary<string, uint>();
                        List<uint> pathStarts = new List<uint>();
                        Dictionary<uint, string> pathLookups = new Dictionary<uint, string>();
                        uint lastNameOffset = 0;
                        int propsSize = this.props.Count * 104;
                        for (int i = 0; i < this.props.Count; i++)
                        {
                            pathStarts.Add(lastNameOffset);
                            this.props[i].Write(bw, propsSize + 4, ref paths, ref lastNameOffset);
                        }
                        foreach (var item in paths)
                        {
                            pathLookups.Add(item.Value, item.Key);
                        }
                        for (int i = 0; i < pathStarts.Count; i++)
                        {
                            bw.WriteWString(pathLookups[pathStarts[i]]);
                        }
                        long subEnd = bw.BaseStream.Position;
                        uint chnkSize = (uint)(subEnd - subStart);
                        bw.BaseStream.Position = chnkStart + 4;
                        bw.Write(chnkSize);         // Size calculated
                        bw.BaseStream.Position = subEnd;
                    }
                    // TODO : write curts
                    // .. and other stuff
                }
            }
        }
    }
}
