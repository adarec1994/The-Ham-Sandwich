namespace ProjectWS.FileFormats.Tex
{
    public partial class Header
    {
        // Common Data //
        public uint version;                // 0 = Header version 1, >0 = Header version 2
        public int width;                   // pixel, power of 2. Size of the largest mipmap
        public int height;                  // pixel, power of 2. Size of the largest mipmap
        public uint depth;                  // Depth (1)
        public uint sides;                  // Sides (1)
        public int mipCount;                // Number of mip levels in the data
        public uint format;                 // Texture format

        // Header V2 //
        public uint isCompressed;           // If (format==0) 0 = uncompressed, 1 = jpeg compression
        public uint compressionFormat;      // The type of jpeg compression: 1,2,3
        public LayerInfo[]? layerInfos;     // Information for the jpeg decompressor
        public uint imageSizesCount;        // Same as mipcount, also used in jpeg
        public uint[]? imageSizes;          // Size of each jpeg data, since they are variable unlike dxt/raw
        public int unk;

        public TextureType textureType;

        public Header(int width, int height, int mipCount)
        {
            this.width = width;
            this.height = height;
            this.mipCount = mipCount;
        }

        public Header(BinaryReader br)
        {
            int magic = br.ReadInt32();     // GFX0
            if (magic != 0x00474658)
            {
                Console.WriteLine("Wrong tex file magic, this is not a texture file");
                return;
            }

            this.version = br.ReadUInt32();
            this.width = br.ReadInt32();
            this.height = br.ReadInt32();
            this.depth = br.ReadUInt32();
            this.sides = br.ReadUInt32();
            this.mipCount = br.ReadInt32();
            this.format = br.ReadUInt32();

            if (this.version > 0)
            {
                this.isCompressed = br.ReadUInt32();
                this.compressionFormat = br.ReadUInt32();
                this.layerInfos = new LayerInfo[] { new LayerInfo(br), new LayerInfo(br), new LayerInfo(br), new LayerInfo(br) };
                this.imageSizesCount = br.ReadUInt32();
                this.imageSizes = new uint[this.imageSizesCount];
                for (int s = 0; s < this.imageSizes.Length; s++)
                {
                    this.imageSizes[s] = br.ReadUInt32();
                }
                br.BaseStream.Position += (13 - this.imageSizesCount) * 4;
                this.unk = br.ReadInt32();
            }

            // Determine texture type //
            this.textureType = TextureType.Unknown;
            switch (this.format)
            {
                case 0:
                    if (this.isCompressed == 1)
                    {
                        switch (this.compressionFormat)
                        {
                            case 0:
                                // Chroma subsampling &amp; typical jpg color space transformation with one additional color channel
                                this.textureType = TextureType.Jpeg1;
                                break;
                            case 1:
                                // Four color channels and no color space transformation
                                this.textureType = TextureType.Jpeg2;
                                break;
                            case 2:
                                // typical jpg color space transformation with one additional color channel
                                this.textureType = TextureType.Jpeg3;
                                break;
                            default:
                                break;
                        }
                    }
                    else
                    {
                        this.textureType = TextureType.Argb1;
                    }
                    break;
                case 1:
                    this.textureType = TextureType.Argb2;
                    break;
                case 5:
                    this.textureType = TextureType.Rgb;
                    break;
                case 6:
                    this.textureType = TextureType.Grayscale;
                    break;
                case 13:
                    this.textureType = TextureType.DXT1;
                    break;
                case 14:
                    this.textureType = TextureType.DXT3;
                    break;
                case 15:
                    this.textureType = TextureType.DXT5;
                    break;
                default:
                    break;
            }
        }

        public void Write(BinaryWriter bw, List<byte[]> mipData)
        {
            bw.Write(0x00474658);
            bw.Write(this.version);
            bw.Write(this.width);
            bw.Write(this.height);
            bw.Write(this.depth);
            bw.Write(this.sides);
            bw.Write(this.mipCount);
            bw.Write(this.format);

            if (this.version > 0)
            {
                bw.Write(this.isCompressed);
                bw.Write(this.compressionFormat);
                for (int i = 0; i < this.layerInfos?.Length; i++)
                {
                    this.layerInfos[i].Write(bw);
                }
                bw.Write(this.imageSizesCount);

                if (this.imageSizesCount == 0)
                {
                    for (int i = 0; i < 13; i++)
                    {
                        bw.Write((int)0);
                    }
                }
                else
                {
                    for (int i = 0; i < this.imageSizesCount; i++)
                    {
                        //bw.Write(this.imageSizes[i]);
                        bw.Write(mipData[i].Length);
                    }
                    for (int i = 0; i < 13 - this.imageSizesCount; i++)
                    {
                        bw.Write((int)0);
                    }
                }
                bw.Write(this.unk);
            }
        }
    }
}