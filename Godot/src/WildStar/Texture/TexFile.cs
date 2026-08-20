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

    public static bool TryDecode(byte[] bytes, out TexFile file, out string error)
    {
        file = null!;

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

        var levels = new byte[mips][];
        int offset = DataStart;

        for (int i = 0; i < mips; i++)
        {
            int mip = mips - 1 - i;
            int w = Math.Max(1, width >> mip);
            int h = Math.Max(1, height >> mip);

            try
            {
                levels[mip] = TexDecoder.DecodeLevel(bytes, offset, sizes[i], w, h,
                                                    pixelMode, quality, isConstant,
                                                    constantValue);
            }
            catch (Exception e)
            {
                error = $"level {mip} ({w}x{h}): {e.Message}";
                return false;
            }

            offset += sizes[i];
        }

        file = new TexFile(width, height, mips, pixelMode, levels);
        error = string.Empty;
        return true;
    }
}
