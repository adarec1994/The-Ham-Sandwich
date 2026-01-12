using BCnEncoder.Shared;
using ProjectWS.FileFormats.M3;
using SixLabors.ImageSharp;
using SixLabors.ImageSharp.PixelFormats;
using SixLabors.ImageSharp.Processing;
using System.Diagnostics;
using System.Linq;

namespace ProjectWS.FileFormats.Tex
{
    public class File
    {
        string filePath;
        public bool failedReading;

        public Header? header;
        public List<byte[]>? mipData;

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
                    
                using(BinaryReader br = new BinaryReader(str))
                {
                    this.header = new Header(br);
                    if (this.header.version > 0)
                        br.BaseStream.Position = 112;

                    this.mipData = new List<byte[]>();

                    switch (this.header.textureType)
                    {
                        case TextureType.Jpeg1:
                        case TextureType.Jpeg2:
                        case TextureType.Jpeg3:
                            {
                                byte[][] jpegMips = new byte[this.header.imageSizesCount][];
                                byte[][] rgbaMips = new byte[this.header.imageSizesCount][];
                                Jpeg.Decoder lt = new Jpeg.Decoder();
                                for (int i = 0; i < this.header.imageSizesCount; i++)
                                {
                                    // Hack (only doing biggest mip, rest are generated)
                                    if (i == this.header.imageSizesCount - 1)
                                    {
                                        jpegMips[i] = br.ReadBytes((int)this.header.imageSizes[i]);
                                        rgbaMips[i] = lt.DecompressMip(this.header, ((int)this.header.imageSizesCount - 1) - i, jpegMips[i]);
                                        this.mipData.Add(rgbaMips[i]);
                                    }
                                    else
                                    {
                                        br.BaseStream.Position += this.header.imageSizes[i];
                                        this.mipData.Add(new byte[0]);
                                    }

                                    //jpegMips[i] = br.ReadBytes((int)this.header.imageSizes[i]);
                                    //rgbaMips[i] = lt.DecompressMip(this.header, ((int)this.header.imageSizesCount - 1) - i, jpegMips[i]);
                                    //this.mipData.Add(rgbaMips[i]);
                                }
                            }
                            break;
                        case TextureType.Argb1:
                        case TextureType.Argb2:
                            {
                                int remainingSize = (int)(br.BaseStream.Length - br.BaseStream.Position);
                                var rawData = br.ReadBytes(remainingSize);

                                int offs = 0;
                                for (int i = 0; i < this.header.imageSizesCount; i++)
                                {
                                    int size = (int)this.header.imageSizes[i];
                                    byte[] rgbaMip = new byte[size];
                                    Buffer.BlockCopy(rawData, offs, rgbaMip, 0, size);
                                    this.mipData.Add(rgbaMip);
                                    offs += size;
                                }
                            }
                            break;
                        case TextureType.Rgb:
                            break;
                        case TextureType.Grayscale:
                            break;
                        case TextureType.DXT1:
                            {
                                int[] DXTSizes = CalculateDXTSizes(this.header.mipCount, this.header.width, this.header.height, 8);

                                for (int d = 0; d < this.header.mipCount; d++)
                                {
                                    byte[] buffer = br.ReadBytes(DXTSizes[d]);
                                    this.mipData.Add(buffer);
                                }
                            }
                            break;
                        case TextureType.DXT3:
                            break;
                        case TextureType.DXT5:
                            {
                                int[] DXTSizes = CalculateDXTSizes(this.header.mipCount, this.header.width, this.header.height, 16);

                                for (int d = 0; d < this.header.mipCount; d++)
                                {
                                    byte[] buffer = br.ReadBytes(DXTSizes[d]);
                                    this.mipData.Add(buffer);
                                }
                            }
                            break;
                        case TextureType.Unknown:
                        default:
                            Console.WriteLine("Unsupported texture type.");
                            return;
                    }
                }
            }
            catch(Exception e)
            {
                Console.WriteLine(e.Message);
            }
        }

        public void ConvertMipDataToDXT()
        {
            if (this.mipData == null || this.header == null)
                return;

            byte[] data = this.mipData[this.mipData.Count - 1];

            BCnEncoder.Encoder.BcEncoder enc = new BCnEncoder.Encoder.BcEncoder(BCnEncoder.Shared.CompressionFormat.Bc3);

            ReadOnlySpan<byte> decoded = data.AsSpan();
            byte[][] mips = enc.EncodeToRawBytes(decoded, this.header.width, this.header.height, BCnEncoder.Encoder.PixelFormat.Rgba32);

            for (int i = 0; i < mips.Length; i++)
            {
                this.mipData[i] = mips[mips.Length - 1 - i];
            }
        }

        public void Write(string filePath)
        {
            if (this.header == null)
                return;

            if (this.filePath == null)
                return;

            if (this.mipData == null)
                return;

            string fileName = Path.GetFileName(filePath);
            string directory = Path.GetDirectoryName(filePath);
            if (!Directory.Exists(directory))
                Directory.CreateDirectory(directory);

            // Change to ARGB
            /*
            this.header.isCompressed = 0;
            this.header.format = 0;
            var currentTextureType = this.header.textureType;
            this.header.imageSizesCount = 0;
            this.header.layerInfos = new LayerInfo[] { new LayerInfo(), new LayerInfo(), new LayerInfo(), new LayerInfo() };
            */
            // Change to DXT5
            this.header.isCompressed = 0;
            this.header.format = 15;
            var currentTextureType = this.header.textureType;
            this.header.imageSizesCount = 0;
            //this.header.mipCount = 1;
            this.header.layerInfos = new LayerInfo[] { new LayerInfo(), new LayerInfo(), new LayerInfo(), new LayerInfo() };

            using (var str = System.IO.File.OpenWrite(filePath))
            {
                using(var bw = new BinaryWriter(str))
                {
                    this.header.Write(bw, this.mipData);

                    switch (currentTextureType)
                    {
                        case TextureType.Jpeg1:
                        case TextureType.Jpeg2:
                        case TextureType.Jpeg3:
                            {
                                for (int i = 0; i < this.mipData.Count; i++)
                                {
                                    bw.Write(this.mipData[i]);
                                }
                            }
                            break;
                        case TextureType.Argb1:
                        case TextureType.Argb2:
                            {
                                // rawdata
                            }
                            break;
                        case TextureType.Rgb:
                            break;
                        case TextureType.Grayscale:
                            break;
                        case TextureType.DXT1:
                            {
                                // rawdata
                            }
                            break;
                        case TextureType.DXT3:
                            break;
                        case TextureType.DXT5:
                            {
                                // rawdata
                            }
                            break;
                        case TextureType.Unknown:
                        default:
                            Console.WriteLine("Unsupported texture type.");
                            return;
                    }
                }
            }
        }

        int[] CalculateDXTSizes(int miplevels, int width, int height, int blockSize)
        {
            int[] DXTSizes = new int[miplevels];
            int increment = 0;
            for (int m = miplevels - 1; m >= 0; m--)
            {
                int w = (int)(width / Math.Pow(2, m));
                int h = (int)(height / Math.Pow(2, m));
                DXTSizes[increment] = (int)(((w + 3) / 4) * ((h + 3) / 4) * blockSize);
                increment++;
            }
            return DXTSizes;
        }
    }
}