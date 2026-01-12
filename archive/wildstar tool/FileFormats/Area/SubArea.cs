using BCnEncoder.Decoder;
using BCnEncoder.Shared;
using Microsoft.Toolkit.HighPerformance;
using ProjectWS.FileFormats.Extensions;
using System.Runtime.InteropServices;
using System;

namespace ProjectWS.FileFormats.Area
{
    public partial class SubArea
    {
        public int index;
        public Flags flags;
        public ushort[] heightMap;
        public uint[] worldLayerIDs;
        public byte[] blendMap;
        public MapMode blendMapMode;
        public byte[] colorMap;
        public MapMode colorMapMode;
        public ushort[] unknownMap;
        public int unk;
        public SkyCorner[] skyCorners;
        public ushort[] lodHeightMap;
        public ushort[] lodHeightRange;     // [0] Minimum, [1] Maximum
        public byte[] unknownMap2;  // Seems to be a layer blend adjust map (gets added to blendMap in shader)
        public uint[] zoneIDs;

        public List<uint> propUniqueIDs;
        public Curd curd;                   // https://cdn.discordapp.com/attachments/487618232279105540/924871987392823327/unknown.png
        public bool hasWater;
        public Water[] waters;

        public enum MapMode
        {
            Raw,
            DXT1,
            DXT5,
        }

        public SubArea(BinaryReader br, int index, bool areaCompressed)
        {
            uint subchunkSize = br.ReadUInt32();
            if (!areaCompressed)
                subchunkSize = subchunkSize & 0xFFFFFF;

            long save = br.BaseStream.Position;
            this.index = index;

            this.flags = (Flags)br.ReadUInt32();

            // Height Map //
            if (this.flags.HasFlag(Flags.hasHeightmap))
            {
                this.heightMap = new ushort[19 * 19];
                for (int x = 0; x < this.heightMap.Length; x++)
                {
                    var heightValue = br.ReadUInt16();
                    this.heightMap[x] = heightValue;
                }
            }

            // World Layer IDs //
            if (this.flags.HasFlag(Flags.hasWorldLayerIDs))
            {
                this.worldLayerIDs = new uint[4];
                for (int i = 0; i < 4; i++)
                {
                    this.worldLayerIDs[i] = br.ReadUInt32();
                }
            }

            // Blend Map //
            if (this.flags.HasFlag(Flags.hasBlendMap))
            {
                this.blendMapMode = MapMode.Raw;
                this.blendMap = new byte[65 * 65 * 4];
                for (int i = 0; i < 65 * 65; i++)
                {
                    ushort val = br.ReadUInt16();
                    /*
                    int value = (val & (0xF << (i * 4))) >> (i * 4);
                    byte blend = (byte)((value / 15.0f) * 255.0f);
                    this.blendMap[j] |= blend << (8 * i);
                    */
                }
            }

            // Color Map //
            if (this.flags.HasFlag(Flags.hasColorMap))
            {
                this.colorMapMode = MapMode.Raw;
                this.colorMap = new byte[65 * 65 * 4];
                for (int i = 0; i < 65 * 65; i++)
                {
                    ushort val = br.ReadUInt16();
                }

                //uint32 r = value & 0x1F;
                //uint32 g = (value >> 5) & 0x3F;
                //uint32 b = (value >> 11) & 0x1F;
                //r = (uint32) ((r / 31.0f) * 255.0f);
                //g = (uint32) ((g / 63.0f) * 255.0f);
                //b = (uint32) ((b / 31.0f) * 255.0f);
            }

            // Unknown Map //
            if (this.flags.HasFlag(Flags.hasUnkMap))
            {
                this.unknownMap = new ushort[65 * 65];
                for (int i = 0; i < this.unknownMap.Length; i++)
                {
                    this.unknownMap[i] = br.ReadUInt16();
                }
            }

            // Unknown data 4 bytes //
            if (this.flags.HasFlag(Flags.unk0x20))
            {
                unk = br.ReadInt32();
            }

            // World Sky IDs //
            if (this.flags.HasFlag(Flags.hasSkyIDs))
            {
                this.skyCorners = new SkyCorner[4];
                for (int i = 0; i < 4; i++)
                {
                    this.skyCorners[i] = new SkyCorner(br);
                }
            }

            // World Sky Weights //
            if (this.flags.HasFlag(Flags.hasSkyWeights))
            {
                if (this.skyCorners != null)
                {
                    for (int i = 0; i < 4; i++)
                    {
                        this.skyCorners[i].ReadWeights(br);
                    }
                }
                else
                {
                    Console.WriteLine("Sky corners should not be null.");
                }
            }

            // Shadow Map //
            // 4225 bytes
            if (this.flags.HasFlag(Flags.hasShadowMap))
            {
                Console.WriteLine("Shadow map");
                br.ReadBytes(65 * 65);
            }

            // LoD Height Map //
            if (this.flags.HasFlag(Flags.hasLoDHeightMap))
            {
                this.lodHeightMap = new ushort[33 * 33];
                for (int x = 0; x < this.lodHeightMap.Length; x++)
                {
                    this.lodHeightMap[x] = br.ReadUInt16();
                }
            }

            // LoD Height Range //
            if (this.flags.HasFlag(Flags.hasLoDHeightRange))
            {
                this.lodHeightRange = new ushort[2];
                this.lodHeightRange[0] = br.ReadUInt16();
                this.lodHeightRange[1] = br.ReadUInt16();
            }

            // Unknown Data //
            if (this.flags.HasFlag(Flags.unk0x800))
            {
                br.BaseStream.Position += 578;
            }

            // Unknown Data //
            // Single byte
            if (this.flags.HasFlag(Flags.unk0x1000))
            {
                br.ReadByte();
            }

            // Color Map DXT //
            // DXT5 65x65 texture, no mips, clamp
            if (this.flags.HasFlag(Flags.hascolorMapDXT))
            {
                this.colorMapMode = MapMode.DXT5;
                this.colorMap = br.ReadBytes(4624);
            }

            // Unknown Map DXT1 //
            if (this.flags.HasFlag(Flags.hasUnkMap0))
            {
                br.ReadBytes(2312);
            }

            // Unknown Data //
            if (this.flags.HasFlag(Flags.unk0x8000))
            {
                br.BaseStream.Position += 8450;
            }

            // Zone Bounds //
            if (this.flags.HasFlag(Flags.hasZoneBounds))
            {
                br.ReadBytes(64 * 64);
            }

            // Blend Map DXT //
            // DXT1 65x65 texture, no mips, clamp
            if (this.flags.HasFlag(Flags.hasBlendMapDXT))
            {
                this.blendMapMode = MapMode.Raw;
                var blendMapDXT = br.ReadBytes(2312);
                BcDecoder decoder = new BcDecoder();

                //var tex = new BCnTextureData(CompressionFormat.Bc1, 65, 65, blendMapDXT);
                var decoded = decoder.DecodeRaw(blendMapDXT, 65, 65, CompressionFormat.Bc1);
                this.blendMap = MemoryMarshal.Cast<ColorRgba32, byte>(decoded).ToArray();
            }

            // Unknown Map DXT1 //
            if (this.flags.HasFlag(Flags.hasUnkMap1))
            {
                this.unknownMap2 = br.ReadBytes(2312);
            }

            // Unknown Map DXT1 //
            if (this.flags.HasFlag(Flags.hasUnkMap2))
            {
                br.ReadBytes(2312);
            }

            // Unknown Map DXT1 //
            if (this.flags.HasFlag(Flags.hasUnkMap3))
            {
                br.ReadBytes(2312);
            }

            // Flags //
            if (this.flags.HasFlag(Flags.unk0x200000))
            {
                var flags = br.ReadByte();
            }

            // Unknown Data //
            if (this.flags.HasFlag(Flags.unk0x400000))
            {
                for (int i = 0; i < 4; i++)
                {
                    br.ReadInt32();
                }
            }

            // Unknown Data //
            if (this.flags.HasFlag(Flags.unk0x800000))
            {
                br.BaseStream.Position += 16900;
            }

            // Unknown Data //
            if (this.flags.HasFlag(Flags.unk0x1000000))
            {
                br.BaseStream.Position += 8;
            }

            // Unknown Data //
            if (this.flags.HasFlag(Flags.unk0x2000000))
            {
                br.BaseStream.Position += 8450;
            }

            // Unknown Data //
            if (this.flags.HasFlag(Flags.unk0x4000000))
            {
                br.BaseStream.Position += 21316;
            }

            // Unknown Data //
            if (this.flags.HasFlag(Flags.unk0x8000000))
            {
                br.BaseStream.Position += 4096;
            }

            // Zone IDs //
            if (this.flags.HasFlag(Flags.hasZoneIDs))
            {
                this.zoneIDs = new uint[4];
                for (int i = 0; i < 4; i++)
                {
                    this.zoneIDs[i] = br.ReadUInt32();
                }
            }

            // Unknown Data //
            if (this.flags.HasFlag(Flags.unk0x20000000))
            {
                br.BaseStream.Position += 8450;
            }

            // Unknown Data //
            if (this.flags.HasFlag(Flags.unk0x40000000))
            {
                br.BaseStream.Position += 8450;
            }

            // Unknown Map DXT1 //
            if (this.flags.HasFlag(Flags.hasUnkMap4))
            {
                br.ReadBytes(2312);
            }

            while (br.BaseStream.Position < subchunkSize + save - 1)
            {
                ChunkID chunkID = (ChunkID)br.ReadUInt32();
                int chunkSize = br.ReadInt32();
                switch (chunkID)
                {
                    case ChunkID.PROP:
                        {
                            this.propUniqueIDs = new List<uint>();
                            for (int i = 0; i < chunkSize / 4; i++)
                            {
                                this.propUniqueIDs.Add(br.ReadUInt32());
                            }
                        }
                        break;
                    case ChunkID.curD:
                        {
                            this.curd = new Curd(br);
                        }
                        break;
                    case ChunkID.WAtG:
                        {
                            int count = br.ReadInt32();
                            if (count > 0)
                            {
                                this.waters = new Water[count];
                                for (int i = 0; i < count; i++)
                                {
                                    this.waters[i] = new Water(br);
                                }
                                this.hasWater = true;
                            }
                        }
                        break;
                    default:
                        br.SkipChunk(chunkID.ToString(), chunkSize, this.GetType().ToString());
                        break;
                }
            }

            br.BaseStream.Position = subchunkSize + save;

            GenerateMissingData();
        }

        public SubArea(int index)
        {
            this.index = index;

            this.flags = Flags.hasHeightmap;

            this.heightMap = new ushort[19 * 19];
            for (int i = 0; i < this.heightMap.Length; i++)
            {
                this.heightMap[i] = 8400;   // !tele 256 -998 256 3538
            }
            //this.flags |= Flags.hasZoneIDs;
            //this.zoneIDs = new uint[4] { 18, 0, 0, 0 };
            /*
            this.flags |= Flags.hasWorldLayerIDs;
            this.worldLayerIDs = new uint[4] { 2, 78, 1041, 673 };

            this.flags |= Flags.hasBlendMapDXT;
            this.blendMap = new byte[65 * 65 * 4];
            for (int i = 0; i < 65 * 65 * 4; i += 4)
            {
                this.blendMap[i] = 255;
                this.blendMap[i + 1] = 0;
                this.blendMap[i + 2] = 0;
                this.blendMap[i + 3] = 0;
            }
            */
            this.flags |= Flags.hasSkyIDs;
            this.flags |= Flags.hasSkyWeights;
            this.skyCorners = new SkyCorner[4];
            for (int i = 0; i < 4; i++)
            {
                var ids = new uint[] { 0, 0, 0, 255 };   // 255 = Adventure_Galeras3.sky
                var weights = new byte[] { 0, 0, 0, 255 };
                this.skyCorners[i] = new SkyCorner(ids, weights);
            }
        }

        public void Write(BinaryWriter bw)
        {
            bw.Write((uint)0);          // Size pad
            long subStart = bw.BaseStream.Position;
            bw.Write((uint)this.flags); // Flags

            // Height Map
            if (this.heightMap != null)
            {
                for (int x = 0; x < this.heightMap.Length; x++)
                {
                    bw.Write(this.heightMap[x]);    // Height value
                }
            }

            // World Layer IDs
            if (this.worldLayerIDs != null)
            {
                for (int i = 0; i < 4; i++)
                {
                    bw.Write(this.worldLayerIDs[i]);
                }
            }

            // Blend Map
            // Color Map
            // UnkMap
            // Unk0x20

            // Sky IDs
            if (this.skyCorners != null)
            {
                for (int i = 0; i < 4; i++)
                {
                    bw.Write(this.skyCorners[i].worldSkyIDs[0]);
                    bw.Write(this.skyCorners[i].worldSkyIDs[1]);
                    bw.Write(this.skyCorners[i].worldSkyIDs[2]);
                    bw.Write(this.skyCorners[i].worldSkyIDs[3]);
                }
            }

            // Sky Weights
            if (this.skyCorners != null)
            {
                for (int i = 0; i < 4; i++)
                {
                    bw.Write(this.skyCorners[i].worldSkyWeights[0]);
                    bw.Write(this.skyCorners[i].worldSkyWeights[1]);
                    bw.Write(this.skyCorners[i].worldSkyWeights[2]);
                    bw.Write(this.skyCorners[i].worldSkyWeights[3]);
                }
            }

            // Shadow Map
            // LoD Height Map
            // LoD Height Range
            // Unk0x800
            // Unk0x1000
            // Color Map DXT
            // Unk Map 0 DXT
            // UnkData 0x8000
            // Zone Bounds

            // Blend Map DXT
            if (this.blendMap != null)
                bw.Write(this.blendMap);

            // Unk Map 1 DXT
            // Unk Map 2 DXT
            // Unk Map 3 DXT
            // Unk Flags
            // Unk Data 0x400000
            // Unk Data 0x800000
            // Unk Data 0x1000000
            // Unk Data 0x2000000
            // Unk Data 0x4000000
            // Unk Data 0x8000000

            // Zone IDs
            if (this.zoneIDs != null)
            {
                for (int i = 0; i < 4; i++)
                {
                    bw.Write(this.zoneIDs[i]);
                }
            }

            // Unk Data 0x20000000
            // Unk Data 0x400000000
            // UnkMap4 DXT

            if (this.propUniqueIDs != null && this.propUniqueIDs.Count > 0)
            {
                bw.Write(1347571536);                       // PROP
                bw.Write(this.propUniqueIDs.Count * 4);     // Size
                for (int i = 0; i < this.propUniqueIDs.Count; i++)
                {
                    bw.Write(this.propUniqueIDs[i]);
                }
            }

            long subEnd = bw.BaseStream.Position;
            uint subSize = (uint)(subEnd - subStart);
            bw.BaseStream.Position = subStart - 4;
            bw.Write(subSize);          // Size calculated
            bw.BaseStream.Position = subEnd;
        }

        void GenerateMissingData()
        {
            if (this.blendMap == null)
            {
                this.blendMapMode = MapMode.Raw;
                this.blendMap = new byte[65 * 65 * 4];
                byte fill = 255;
                for (int i = 0; i < 65 * 65; i++)
                {
                    this.blendMap[i * 4 + 0] = fill;
                    this.blendMap[i * 4 + 1] = 0;
                    this.blendMap[i * 4 + 2] = 0;
                    this.blendMap[i * 4 + 3] = 0;
                }
            }

            if (this.colorMap == null)
            {
                this.colorMapMode = MapMode.Raw;
                this.colorMap = new byte[65 * 65 * 4];
                byte fill = 255;
                byte half = 128;
                for (int i = 0; i < 65 * 65; i++)
                {
                    this.colorMap[i * 4 + 0] = half;
                    this.colorMap[i * 4 + 1] = half;
                    this.colorMap[i * 4 + 2] = half;
                    this.colorMap[i * 4 + 3] = fill;
                }
            }
        }
    }
}
