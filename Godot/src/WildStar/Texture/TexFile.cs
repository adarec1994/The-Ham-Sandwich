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

    public readonly struct FormatDesc
    {
        public FormatDesc(int widthAdd, int widthShift, int heightAdd, int heightShift,
                          int depthAdd, int depthShift, int bytesPerBlock)
        {
            WidthAdd = widthAdd;
            WidthShift = widthShift;
            HeightAdd = heightAdd;
            HeightShift = heightShift;
            DepthAdd = depthAdd;
            DepthShift = depthShift;
            BytesPerBlock = bytesPerBlock;
        }

        public int WidthAdd { get; }
        public int WidthShift { get; }
        public int HeightAdd { get; }
        public int HeightShift { get; }
        public int DepthAdd { get; }
        public int DepthShift { get; }
        public int BytesPerBlock { get; }
    }

    private static readonly FormatDesc[] Formats =
    {
        new(0, 0, 0, 0, 0, 0, 4),
        new(0, 0, 0, 0, 0, 0, 4),
        new(0, 0, 0, 0, 0, 0, 2),
        new(0, 0, 0, 0, 0, 0, 2),
        new(0, 0, 0, 0, 0, 0, 2),
        new(0, 0, 0, 0, 0, 0, 2),
        new(0, 0, 0, 0, 0, 0, 1),
        new(0, 0, 0, 0, 0, 0, 2),
        new(0, 0, 0, 0, 0, 0, 4),
        new(0, 0, 0, 0, 0, 0, 8),
        new(0, 0, 0, 0, 0, 0, 2),
        new(0, 0, 0, 0, 0, 0, 4),
        new(0, 0, 0, 0, 0, 0, 4),
        new(3, 2, 3, 2, 0, 0, 8),
        new(3, 2, 3, 2, 0, 0, 16),
        new(3, 2, 3, 2, 0, 0, 16),
        new(0, 0, 0, 0, 0, 0, 2),
        new(0, 0, 0, 0, 0, 0, 4),
        new(0, 0, 0, 0, 0, 0, 8),
        new(0, 0, 0, 0, 0, 0, 4),
    };

    public const int FormatBgrx8888 = 1;
    public const int FormatBgra8888 = 0;
    public const int FormatDxt1 = 13;
    public const int FormatDxt3 = 14;
    public const int FormatDxt5 = 15;

    private static int LevelSize(in FormatDesc f, int width, int height, int depth, int faces)
    {
        int w = Math.Max(width, 1);
        int h = Math.Max(height, 1);
        int d = Math.Max(depth, 1);

        long row = ((long)f.BytesPerBlock * ((w + f.WidthAdd) >> f.WidthShift) + 3) & ~3L;
        long size = row * faces
                        * ((h + f.HeightAdd) >> f.HeightShift)
                        * ((f.DepthAdd + d) >> f.DepthShift);

        return size > int.MaxValue ? -1 : (int)size;
    }

    private readonly struct Header
    {
        public Header(int width, int height, int mips, int pixelMode, int formatIndex,
                      int storageMode, int faces, int depth, int[] quality,
                      bool[] isConstant, int[] constantValue, int[] sizes)
        {
            Width = width;
            Height = height;
            Mips = mips;
            PixelMode = pixelMode;
            FormatIndex = formatIndex;
            StorageMode = storageMode;
            Faces = faces;
            Depth = depth;
            Quality = quality;
            IsConstant = isConstant;
            ConstantValue = constantValue;
            Sizes = sizes;
        }

        public int Width { get; }
        public int Height { get; }
        public int Mips { get; }
        public int PixelMode { get; }
        public int FormatIndex { get; }
        public int StorageMode { get; }
        public int Faces { get; }
        public int Depth { get; }
        public int[] Quality { get; }
        public bool[] IsConstant { get; }
        public int[] ConstantValue { get; }
        public int[] Sizes { get; }
    }

    private static bool TryReadHeader(byte[] bytes, out Header header, out string error) =>
        TryReadHeader(bytes, out header, out error, requireAllData: true);

    private static bool TryReadHeader(byte[] bytes, out Header header, out string error,
                                      bool requireAllData)
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

        if (storageMode != 0 && storageMode != 1)
        {
            error = "storage mode " + storageMode + " not supported";
            return false;
        }

        int formatIndex = (int)BinaryPrimitives.ReadUInt32LittleEndian(bytes.AsSpan(0x1C));

        if (storageMode == 0)
        {
            if (formatIndex < 0 || formatIndex >= Formats.Length)
            {
                error = "unknown texture format index " + formatIndex;
                return false;
            }

            if (!TexRaw.Supports(formatIndex))
            {
                error = "texture format index " + formatIndex + " not supported";
                return false;
            }
        }

        if (storageMode == 1 && (pixelMode < 0 || pixelMode > 2))
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

        var sizes = new int[mips];
        long total = 0;

        if (storageMode == 1)
        {
            if (DataStart + 4L * mips > bytes.Length)
            {
                error = "level size table runs past the end of the file";
                return false;
            }

            for (int i = 0; i < mips; i++)
            {
                sizes[i] = (int)BinaryPrimitives.ReadUInt32LittleEndian(bytes.AsSpan(0x38 + i * 4));
                total += sizes[i];
            }
        }
        else
        {
            FormatDesc format = Formats[formatIndex];
            for (int i = 0; i < mips; i++)
            {
                int mip = mips - 1 - i;
                sizes[i] = LevelSize(format, width >> mip, height >> mip, depth >> mip, faces);
                if (sizes[i] < 0)
                {
                    error = "implausible computed level size";
                    return false;
                }

                total += sizes[i];
            }
        }

        if (requireAllData && DataStart + total > bytes.Length)
        {
            error = "level data runs past the end of the file";
            return false;
        }

        header = new Header(width, height, mips, pixelMode, formatIndex, storageMode,
                            faces, depth, quality, isConstant, constantValue, sizes);
        error = string.Empty;
        return true;
    }

    public static bool TryGetRawDxt(byte[] bytes, out int width, out int height, out int format,
                                    out bool mipmapped, out byte[] data)
    {
        width = 0;
        height = 0;
        format = 0;
        mipmapped = false;
        data = System.Array.Empty<byte>();

        if (!TryReadHeader(bytes, out Header header, out _))
        {
            return false;
        }

        if (header.StorageMode != 0 ||
            (header.FormatIndex != FormatDxt1 && header.FormatIndex != FormatDxt3 &&
             header.FormatIndex != FormatDxt5))
        {
            return false;
        }

        int full = 1;
        while ((Math.Max(header.Width, header.Height) >> full) >= 1)
        {
            full++;
        }

        mipmapped = header.Mips > 1;
        if (mipmapped && header.Mips != full)
        {
            return false;
        }

        int total = 0;
        for (int i = 0; i < header.Mips; i++)
        {
            total += header.Sizes[i];
        }

        if (DataStart + total > bytes.Length)
        {
            return false;
        }

        data = new byte[total];
        int offset = DataStart;
        var levelOffsets = new int[header.Mips];
        int at = total;
        for (int i = 0; i < header.Mips; i++)
        {
            at -= header.Sizes[i];
            levelOffsets[i] = at;
        }

        for (int i = 0; i < header.Mips; i++)
        {
            System.Buffer.BlockCopy(bytes, offset, data, levelOffsets[i], header.Sizes[i]);
            offset += header.Sizes[i];
        }

        width = header.Width;
        height = header.Height;
        format = header.FormatIndex;
        return true;
    }

    public static bool TryDecode(byte[] bytes, out TexFile file, out string error) =>
        TryDecode(bytes, int.MaxValue, out file, out error);

    public static bool TryDecode(byte[] bytes, int maxSize, out TexFile file, out string error)
    {
        file = null!;

        if (!TryReadHeader(bytes, out Header header, out error))
        {
            return false;
        }

        int skip = 0;
        while (skip < header.Mips - 1 &&
               Math.Max(Math.Max(1, header.Width >> skip), Math.Max(1, header.Height >> skip)) > maxSize)
        {
            skip++;
        }

        var levels = new byte[header.Mips - skip][];
        int offset = DataStart;

        for (int i = 0; i < header.Mips; i++)
        {
            int mip = header.Mips - 1 - i;
            if (mip < skip)
            {
                break;
            }

            int w = Math.Max(1, header.Width >> mip);
            int h = Math.Max(1, header.Height >> mip);

            try
            {
                levels[mip - skip] = header.StorageMode == 0
                    ? TexRaw.DecodeLevel(bytes, offset, header.Sizes[i], w, h, header.FormatIndex)
                    : TexDecoder.DecodeLevel(bytes, offset, header.Sizes[i], w, h,
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

        file = new TexFile(Math.Max(1, header.Width >> skip), Math.Max(1, header.Height >> skip),
                           header.Mips - skip, header.PixelMode, levels);
        error = string.Empty;
        return true;
    }

    private static int ThumbnailMip(in Header header, int targetSize)
    {
        for (int candidate = header.Mips - 1; candidate >= 0; candidate--)
        {
            int w = Math.Max(1, header.Width >> candidate);
            int h = Math.Max(1, header.Height >> candidate);
            if (Math.Max(w, h) >= targetSize)
            {
                return candidate;
            }
        }

        return 0;
    }

    public static bool TryThumbnailExtent(byte[] head, int targetSize, out int bytesNeeded,
                                          out string error)
    {
        bytesNeeded = 0;
        if (!TryReadHeader(head, out Header header, out error, requireAllData: false))
        {
            return false;
        }

        int index = header.Mips - 1 - ThumbnailMip(header, targetSize);
        int need = DataStart;
        for (int i = 0; i <= index; i++)
        {
            need += header.Sizes[i];
        }

        bytesNeeded = need;
        return true;
    }

    public static bool TryDecodeThumbnail(byte[] bytes, int targetSize, out int width,
                                          out int height, out byte[] rgba, out string error)
    {
        width = 0;
        height = 0;
        rgba = Array.Empty<byte>();

        if (!TryReadHeader(bytes, out Header header, out error, requireAllData: false))
        {
            return false;
        }

        int mip = ThumbnailMip(header, targetSize);

        int offset = DataStart;
        for (int i = 0; i < header.Mips - 1 - mip; i++)
        {
            offset += header.Sizes[i];
        }

        int index = header.Mips - 1 - mip;
        width = Math.Max(1, header.Width >> mip);
        height = Math.Max(1, header.Height >> mip);

        if (offset + (long)header.Sizes[index] > bytes.Length)
        {
            error = $"level {mip} ({width}x{height}) runs past the {bytes.Length} bytes given";
            return false;
        }

        try
        {
            rgba = header.StorageMode == 0
                ? TexRaw.DecodeLevel(bytes, offset, header.Sizes[index], width, height,
                                     header.FormatIndex)
                : TexDecoder.DecodeLevel(bytes, offset, header.Sizes[index], width, height,
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
