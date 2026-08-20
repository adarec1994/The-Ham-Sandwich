using System;
using System.Buffers.Binary;

namespace WildStar.Texture;

public sealed class TexFile
{
    public const uint Magic = 0x00474658;
    public const int Version = 3;
    public const int DataStart = 112;

    private TexFile(int width, int height, int mipCount, int pixelMode, byte[][] levels)
    {
        Width = width;
        Height = height;
        MipCount = mipCount;
        PixelMode = pixelMode;
        Levels = levels;
    }

    public int Width { get; }

    public int Height { get; }

    public int MipCount { get; }

    public int PixelMode { get; }

    public byte[][] Levels { get; }

    public const int NormalPixelMode = 1;

    public bool IsNormalMap => PixelMode == NormalPixelMode;

    public static void RepackNormal(byte[] rgba)
    {
        for (int i = 0; i < rgba.Length; i += 4)
        {
            float nx = rgba[i + 3] / 255.0f * 2.0f - 1.0f;
            float ny = rgba[i + 1] / 255.0f * 2.0f - 1.0f;
            float nz = MathF.Sqrt(Math.Clamp(1.0f - nx * nx - ny * ny, 0.0f, 1.0f));

            rgba[i] = (byte)Math.Clamp((nx * 0.5f + 0.5f) * 255.0f, 0.0f, 255.0f);
            rgba[i + 1] = (byte)Math.Clamp((ny * 0.5f + 0.5f) * 255.0f, 0.0f, 255.0f);
            rgba[i + 2] = (byte)Math.Clamp((nz * 0.5f + 0.5f) * 255.0f, 0.0f, 255.0f);
            rgba[i + 3] = 255;
        }
    }

    private readonly struct Header
    {
        public Header(int width, int height, int mips, int pixelMode, int[] quality,
                      bool[] isConstant, int[] constantValue, int[] sizes)
        {
            Width = width;
            Height = height;
            Mips = mips;
            PixelMode = pixelMode;
            Quality = quality;
            IsConstant = isConstant;
            ConstantValue = constantValue;
            Sizes = sizes;
        }

        public int Width { get; }
        public int Height { get; }
        public int Mips { get; }
        public int PixelMode { get; }
        public int[] Quality { get; }
        public bool[] IsConstant { get; }
        public int[] ConstantValue { get; }
        public int[] Sizes { get; }
    }

    private static bool TryReadHeader(byte[] bytes, out Header header, out string error)
    {
        header = default;

        if (bytes.Length < DataStart)
        {
            error = "shorter than a GFX header";
            return false;
        }

        if (BinaryPrimitives.ReadUInt32LittleEndian(bytes) != Magic)
        {
            error = "missing GFX signature";
            return false;
        }

        int version = (int)BinaryPrimitives.ReadUInt32LittleEndian(bytes.AsSpan(4));
        if (version != Version)
        {
            error = "unsupported GFX version " + version;
            return false;
        }

        int width = (int)BinaryPrimitives.ReadUInt32LittleEndian(bytes.AsSpan(0x08));
        int height = (int)BinaryPrimitives.ReadUInt32LittleEndian(bytes.AsSpan(0x0C));
        int depth = (int)BinaryPrimitives.ReadUInt32LittleEndian(bytes.AsSpan(0x10));
        int faces = (int)BinaryPrimitives.ReadUInt32LittleEndian(bytes.AsSpan(0x14));
        int mips = (int)BinaryPrimitives.ReadUInt32LittleEndian(bytes.AsSpan(0x18));
        int storageMode = (int)BinaryPrimitives.ReadUInt32LittleEndian(bytes.AsSpan(0x20));
        int pixelMode = (int)BinaryPrimitives.ReadUInt32LittleEndian(bytes.AsSpan(0x24));

        if (width <= 0 || height <= 0 || mips <= 0 || mips > 12)
        {
            error = $"implausible dimensions {width}x{height} mips={mips}";
            return false;
        }

        if (depth != 1 || faces != 1)
        {
            error = "only 2D textures are supported (depth/faces != 1)";
            return false;
        }

        if (storageMode != 1)
        {
            error = "storage mode " + storageMode + " (computed level sizes) not supported";
            return false;
        }

        if (pixelMode != 0 && pixelMode != 1)
        {
            error = "pixel mode " + pixelMode + " not supported";
            return false;
        }

        var quality = new int[4];
        var isConstant = new bool[4];
        var constantValue = new int[4];
        for (int c = 0; c < 4; c++)
        {
            quality[c] = bytes[0x28 + c * 3];
            isConstant[c] = bytes[0x29 + c * 3] != 0;
            constantValue[c] = bytes[0x2A + c * 3];
        }

        if (DataStart + 4L * mips > bytes.Length)
        {
            error = "level size table runs past the end of the file";
            return false;
        }

        var sizes = new int[mips];
        long total = 0;
        for (int i = 0; i < mips; i++)
        {
            sizes[i] = (int)BinaryPrimitives.ReadUInt32LittleEndian(bytes.AsSpan(0x38 + i * 4));
            total += sizes[i];
        }

        if (DataStart + total > bytes.Length)
        {
            error = "level data runs past the end of the file";
            return false;
        }

        header = new Header(width, height, mips, pixelMode, quality, isConstant,
                            constantValue, sizes);
        error = string.Empty;
        return true;
    }

    public static bool TryDecode(byte[] bytes, out TexFile file, out string error)
    {
        file = null!;

        if (!TryReadHeader(bytes, out Header header, out error))
        {
            return false;
        }

        var levels = new byte[header.Mips][];
        int offset = DataStart;

        for (int i = 0; i < header.Mips; i++)
        {
            int mip = header.Mips - 1 - i;
            int w = Math.Max(1, header.Width >> mip);
            int h = Math.Max(1, header.Height >> mip);

            try
            {
                levels[mip] = TexDecoder.DecodeLevel(bytes, offset, header.Sizes[i], w, h,
                                                    header.PixelMode, header.Quality,
                                                    header.IsConstant, header.ConstantValue);
            }
            catch (Exception e)
            {
                error = $"level {mip} ({w}x{h}): {e.Message}";
                return false;
            }

            offset += header.Sizes[i];
        }

        file = new TexFile(header.Width, header.Height, header.Mips, header.PixelMode, levels);
        error = string.Empty;
        return true;
    }

    public static bool TryDecodeThumbnail(byte[] bytes, int targetSize, out int width,
                                          out int height, out byte[] rgba, out string error)
    {
        width = 0;
        height = 0;
        rgba = Array.Empty<byte>();

        if (!TryReadHeader(bytes, out Header header, out error))
        {
            return false;
        }

        int mip = 0;
        for (int candidate = header.Mips - 1; candidate >= 0; candidate--)
        {
            int w = Math.Max(1, header.Width >> candidate);
            int h = Math.Max(1, header.Height >> candidate);
            if (Math.Max(w, h) >= targetSize)
            {
                mip = candidate;
                break;
            }
        }

        int offset = DataStart;
        for (int i = 0; i < header.Mips - 1 - mip; i++)
        {
            offset += header.Sizes[i];
        }

        int index = header.Mips - 1 - mip;
        width = Math.Max(1, header.Width >> mip);
        height = Math.Max(1, header.Height >> mip);

        try
        {
            rgba = TexDecoder.DecodeLevel(bytes, offset, header.Sizes[index], width, height,
                                          header.PixelMode, header.Quality,
                                          header.IsConstant, header.ConstantValue);
        }
        catch (Exception e)
        {
            error = $"level {mip} ({width}x{height}): {e.Message}";
            return false;
        }

        if (header.PixelMode == NormalPixelMode)
        {
            RepackNormal(rgba);
        }

        error = string.Empty;
        return true;
    }
}
