using System;
using System.Buffers.Binary;

namespace WildStar.Model;

public static class M3ParticleTrack
{
    public static float Half(M3RawTrack track, uint time, float fallback = 0.0f)
    {
        if (!Locate(track, 2, time, out int a, out int b, out float t))
        {
            return fallback;
        }

        float va = HalfBits.ToSingle(BinaryPrimitives.ReadUInt16LittleEndian(track.Values.AsSpan(a * 2)));
        float vb = HalfBits.ToSingle(BinaryPrimitives.ReadUInt16LittleEndian(track.Values.AsSpan(b * 2)));
        return va + (vb - va) * t;
    }

    public static int U16(M3RawTrack track, uint time, int fallback = 0)
    {
        if (!Locate(track, 2, time, out int a, out int b, out float t))
        {
            return fallback;
        }

        int va = BinaryPrimitives.ReadUInt16LittleEndian(track.Values.AsSpan(a * 2));
        int vb = BinaryPrimitives.ReadUInt16LittleEndian(track.Values.AsSpan(b * 2));
        return (int)MathF.Round(va + (vb - va) * t);
    }

    public static int U32(M3RawTrack track, uint time, int fallback = 0)
    {
        if (!Locate(track, 4, time, out int a, out int b, out float t))
        {
            return fallback;
        }

        long va = BinaryPrimitives.ReadUInt32LittleEndian(track.Values.AsSpan(a * 4));
        long vb = BinaryPrimitives.ReadUInt32LittleEndian(track.Values.AsSpan(b * 4));
        return (int)Math.Round(va + (vb - va) * t);
    }

    public static float U8(M3RawTrack track, uint time, float fallback = 0.0f)
    {
        if (!Locate(track, 1, time, out int a, out int b, out float t))
        {
            return fallback;
        }

        float va = track.Values[a] * (1.0f / 255.0f);
        float vb = track.Values[b] * (1.0f / 255.0f);
        return va + (vb - va) * t;
    }

    public static int U8Step(M3RawTrack track, uint time, int fallback = 0)
    {
        int count = track.Count;
        if (count == 0 || track.Values.Length < count)
        {
            return fallback;
        }

        uint[] keys = track.Keys;
        uint last = keys[count - 1];
        if (count > 1 && last > 0)
        {
            time %= last + 1;
        }

        int index = 0;
        while (index + 1 < count && keys[index + 1] <= time)
        {
            index++;
        }

        return track.Values[index];
    }

    public static (float X, float Y, float Z) Half3(M3RawTrack track, uint time)
    {
        if (!Locate(track, 6, time, out int a, out int b, out float t))
        {
            return (0.0f, 0.0f, 0.0f);
        }

        ReadOnlySpan<byte> pa = track.Values.AsSpan(a * 6);
        ReadOnlySpan<byte> pb = track.Values.AsSpan(b * 6);
        return (Lerp(HalfBits.ToSingle(BinaryPrimitives.ReadUInt16LittleEndian(pa)), HalfBits.ToSingle(BinaryPrimitives.ReadUInt16LittleEndian(pb)), t),
                Lerp(HalfBits.ToSingle(BinaryPrimitives.ReadUInt16LittleEndian(pa[2..])), HalfBits.ToSingle(BinaryPrimitives.ReadUInt16LittleEndian(pb[2..])), t),
                Lerp(HalfBits.ToSingle(BinaryPrimitives.ReadUInt16LittleEndian(pa[4..])), HalfBits.ToSingle(BinaryPrimitives.ReadUInt16LittleEndian(pb[4..])), t));
    }

    public static (float X, float Y, float Z) Float3(M3RawTrack track, uint time)
    {
        if (!Locate(track, 12, time, out int a, out int b, out float t))
        {
            return (0.0f, 0.0f, 0.0f);
        }

        ReadOnlySpan<byte> pa = track.Values.AsSpan(a * 12);
        ReadOnlySpan<byte> pb = track.Values.AsSpan(b * 12);
        return (Lerp(BinaryPrimitives.ReadSingleLittleEndian(pa), BinaryPrimitives.ReadSingleLittleEndian(pb), t),
                Lerp(BinaryPrimitives.ReadSingleLittleEndian(pa[4..]), BinaryPrimitives.ReadSingleLittleEndian(pb[4..]), t),
                Lerp(BinaryPrimitives.ReadSingleLittleEndian(pa[8..]), BinaryPrimitives.ReadSingleLittleEndian(pb[8..]), t));
    }

    public static (float R, float G, float B) Rgb8(M3RawTrack track, uint time)
    {
        if (!Locate(track, 4, time, out int a, out int b, out float t))
        {
            return (1.0f, 1.0f, 1.0f);
        }

        ReadOnlySpan<byte> pa = track.Values.AsSpan(a * 4);
        ReadOnlySpan<byte> pb = track.Values.AsSpan(b * 4);
        const float scale = 1.0f / 255.0f;
        return (Lerp(pa[0] * scale, pb[0] * scale, t), Lerp(pa[1] * scale, pb[1] * scale, t), Lerp(pa[2] * scale, pb[2] * scale, t));
    }

    private static float Lerp(float a, float b, float t) => a + (b - a) * t;

    private static bool Locate(M3RawTrack track, int valueSize, uint time, out int a, out int b, out float t)
    {
        a = 0;
        b = 0;
        t = 0.0f;
        int count = track.Count;
        if (count == 0 || track.Values.Length < count * valueSize)
        {
            return false;
        }

        if (count == 1)
        {
            return true;
        }

        uint[] keys = track.Keys;
        uint last = keys[count - 1];
        if (last > 0)
        {
            time %= last + 1;
        }

        int index = 0;
        while (index + 1 < count && keys[index + 1] <= time)
        {
            index++;
        }

        a = index;
        b = Math.Min(index + 1, count - 1);
        uint ka = keys[a];
        uint kb = keys[b];
        t = kb > ka ? (float)(time - ka) / (kb - ka) : 0.0f;
        return true;
    }
}

public sealed class M3ParticleCurve
{
    public const int MaxKeys = 5;
    public const int BakedSamples = 10;

    public M3ParticleCurve(int keyCount, M3RawTrack[] ages, M3RawTrack[] values, int valueSize)
    {
        KeyCount = Math.Clamp(keyCount, 0, MaxKeys);
        Ages = ages;
        Values = values;
        ValueSize = valueSize;
    }

    public int KeyCount { get; }

    public M3RawTrack[] Ages { get; }

    public M3RawTrack[] Values { get; }

    public int ValueSize { get; }

    public bool IsEmpty => KeyCount == 0;

    public float Sample(float t, uint time, float fallback)
    {
        int n = KeyCount;
        if (n == 0)
        {
            return fallback;
        }

        Span<float> ages = stackalloc float[MaxKeys];
        Span<float> values = stackalloc float[MaxKeys];
        for (int i = 0; i < n; i++)
        {
            ages[i] = M3ParticleTrack.U8(Ages[i], time);
            values[i] = ValueSize == 1 ? M3ParticleTrack.U8(Values[i], time) : M3ParticleTrack.Half(Values[i], time);
        }

        return Evaluate(t, ages[..n], values[..n]);
    }

    public (float R, float G, float B) SampleColour(float t, uint time)
    {
        int n = KeyCount;
        if (n == 0 || ValueSize != 4)
        {
            return (1.0f, 1.0f, 1.0f);
        }

        Span<float> ages = stackalloc float[MaxKeys];
        Span<float> r = stackalloc float[MaxKeys];
        Span<float> g = stackalloc float[MaxKeys];
        Span<float> b = stackalloc float[MaxKeys];
        for (int i = 0; i < n; i++)
        {
            ages[i] = M3ParticleTrack.U8(Ages[i], time);
            (r[i], g[i], b[i]) = M3ParticleTrack.Rgb8(Values[i], time);
        }

        return (Evaluate(t, ages[..n], r[..n]), Evaluate(t, ages[..n], g[..n]), Evaluate(t, ages[..n], b[..n]));
    }

    public static float Evaluate(float t, ReadOnlySpan<float> ages, ReadOnlySpan<float> values)
    {
        int n = ages.Length;
        if (n == 0)
        {
            return 0.0f;
        }

        int segment = 0;
        for (int i = 1; i < n; i++)
        {
            if (t < ages[i])
            {
                break;
            }

            segment++;
        }

        if (segment == n - 1)
        {
            if (n > 1)
            {
                float a0 = ages[segment - 1];
                float a1 = ages[segment];
                if (a1 == a0)
                {
                    return values[segment];
                }

                return (values[segment] - values[segment - 1]) / (a1 - a0) * (t - a0) + values[segment - 1];
            }

            return values[segment];
        }

        float from = ages[segment];
        float to = ages[segment + 1];
        if (to != from)
        {
            return (t - from) / (to - from) * (values[segment + 1] - values[segment]) + values[segment];
        }

        return values[segment];
    }
}

public sealed class M3ParticleEmitter
{
    public const int RecordSize = 160;
    public const int BlockASize = 3792;
    public const int VariantCount = 32;

    public const int ShapePoint = 0;
    public const int ShapeLine = 1;
    public const int ShapeRectangle = 3;
    public const int ShapeBox = 4;
    public const int ShapeRing = 5;
    public const int ShapeSphere = 6;

    public const uint FlagAttractToEmitter = 0x1;
    public const uint FlagWorldOffset = 0x2;
    public const uint FlagColourCurveD = 0x8;
    public const uint FlagRandomSpinSign = 0x10;
    public const uint FlagLoopFrames = 0x1000;

    private M3ParticleEmitter()
    {
    }

    public int Bone { get; private set; }

    public int GeosetId { get; private set; }

    public int Class { get; private set; }

    public int Space { get; private set; }

    public int ItemFlags { get; private set; }

    public int BlendMode { get; private set; }

    public int RenderType { get; private set; }

    public bool DepthTest { get; private set; }

    public bool DepthWrite { get; private set; }

    public uint Seed { get; private set; }

    public M3RawTrack Intensity { get; private set; } = M3RawTrack.Empty;

    public M3RawTrack Enable { get; private set; } = M3RawTrack.Empty;

    public bool HasBlock { get; private set; }

    public int Kind { get; private set; }

    public int Shape { get; private set; }

    public int ShapeFlags { get; private set; }

    public M3RawTrack[] ShapeTracks { get; private set; } = Array.Empty<M3RawTrack>();

    public M3RawTrack SpawnCountMin { get; private set; } = M3RawTrack.Empty;

    public M3RawTrack SpawnCountMax { get; private set; } = M3RawTrack.Empty;

    public M3RawTrack IntervalMin { get; private set; } = M3RawTrack.Empty;

    public M3RawTrack IntervalMax { get; private set; } = M3RawTrack.Empty;

    public M3RawTrack LifeMin { get; private set; } = M3RawTrack.Empty;

    public M3RawTrack LifeMax { get; private set; } = M3RawTrack.Empty;

    public M3RawTrack VelocityA { get; private set; } = M3RawTrack.Empty;

    public M3RawTrack VelocityB { get; private set; } = M3RawTrack.Empty;

    public M3RawTrack SpeedMin { get; private set; } = M3RawTrack.Empty;

    public M3RawTrack SpeedMax { get; private set; } = M3RawTrack.Empty;

    public M3RawTrack Acceleration { get; private set; } = M3RawTrack.Empty;

    public M3RawTrack SpreadMin { get; private set; } = M3RawTrack.Empty;

    public M3RawTrack Spread { get; private set; } = M3RawTrack.Empty;

    public M3RawTrack VelocityAScale { get; private set; } = M3RawTrack.Empty;

    public M3RawTrack RadialSpreadMin { get; private set; } = M3RawTrack.Empty;

    public M3RawTrack RadialSpread { get; private set; } = M3RawTrack.Empty;

    public M3RawTrack RadialScale { get; private set; } = M3RawTrack.Empty;

    public M3RawTrack SizeRandomMin { get; private set; } = M3RawTrack.Empty;

    public M3RawTrack SizeRandomMax { get; private set; } = M3RawTrack.Empty;

    public M3RawTrack MaxParticles { get; private set; } = M3RawTrack.Empty;

    public M3RawTrack WorldOffset { get; private set; } = M3RawTrack.Empty;

    public M3ParticleCurve Alpha { get; private set; } = new(0, Array.Empty<M3RawTrack>(), Array.Empty<M3RawTrack>(), 1);

    public M3ParticleCurve Size { get; private set; } = new(0, Array.Empty<M3RawTrack>(), Array.Empty<M3RawTrack>(), 2);

    public M3ParticleCurve Speed { get; private set; } = new(0, Array.Empty<M3RawTrack>(), Array.Empty<M3RawTrack>(), 2);

    public M3ParticleCurve[] Colour { get; private set; } = Array.Empty<M3ParticleCurve>();

    public uint Flags { get; private set; }

    public uint KillFlags { get; private set; }

    public float KillTolerance { get; private set; }

    public float RotationMin { get; private set; }

    public float RotationMax { get; private set; }

    public float SpinMin { get; private set; }

    public float SpinMax { get; private set; }

    public float SpinAccelerationMin { get; private set; }

    public float SpinAccelerationMax { get; private set; }

    public int TextureIndex { get; private set; }

    public int TextureIndex1 { get; private set; }

    public int TextureIndex2 { get; private set; }

    public int TextureIndex4 { get; private set; }

    public bool FlipU { get; private set; }

    public bool FlipV { get; private set; }

    public float FadeMin { get; private set; }

    public float FadeMax { get; private set; }

    public int FrameOffsetMin { get; private set; }

    public int FrameOffsetMax { get; private set; }

    public int FrameTimeMs { get; private set; }

    public float UvScaleX { get; private set; } = 1.0f;

    public float UvScaleY { get; private set; } = 1.0f;

    public int ColumnShift { get; private set; }

    public int FrameCount { get; private set; }

    public int HoldMinMs { get; private set; }

    public int HoldMaxMs { get; private set; }

    public int MoveMinMs { get; private set; }

    public int MoveMaxMs { get; private set; }

    public bool LoopFrames => (Flags & FlagLoopFrames) != 0;

    public bool AttractToEmitter => (Flags & FlagAttractToEmitter) != 0;

    public bool RandomSpinSign => (Flags & FlagRandomSpinSign) != 0;

    public int Columns => 1 << ColumnShift;

    internal static M3ParticleEmitter Read(byte[] bytes, int record, long nested)
    {
        var emitter = new M3ParticleEmitter
        {
            Bone = U16(bytes, record),
            GeosetId = U16(bytes, record + 0x02),
            Class = (int)U32(bytes, record + 0x04),
            Space = (int)U32(bytes, record + 0x08),
            ItemFlags = U16(bytes, record + 0x10),
            BlendMode = U16(bytes, record + 0x18),
            RenderType = (int)U32(bytes, record + 0x1C),
            DepthTest = U32(bytes, record + 0x20) != 0,
            DepthWrite = U32(bytes, record + 0x24) != 0,
            Seed = U32(bytes, record + 0x2C),
            Intensity = Track(bytes, record + 0x38, nested, 2),
            Enable = Track(bytes, record + 0x50, nested, 1),
        };

        ulong blockOffset = BinaryPrimitives.ReadUInt64LittleEndian(bytes.AsSpan(record + 0x90));
        if (blockOffset == ulong.MaxValue || nested + (long)blockOffset + BlockASize > bytes.Length)
        {
            return emitter;
        }

        int a = (int)(nested + (long)blockOffset);
        long trackBase = a + BlockASize;
        emitter.HasBlock = true;
        emitter.Kind = bytes[a];
        emitter.Shape = U16(bytes, a + 0x08);
        emitter.ShapeFlags = U16(bytes, a + 0x0A);
        emitter.ShapeTracks = ReadShape(bytes, a, trackBase, emitter.Shape);
        emitter.SpawnCountMin = Track(bytes, a + 0x068, trackBase, 2);
        emitter.SpawnCountMax = Track(bytes, a + 0x080, trackBase, 2);
        emitter.IntervalMin = Track(bytes, a + 0x098, trackBase, 4);
        emitter.IntervalMax = Track(bytes, a + 0x0B0, trackBase, 4);
        emitter.LifeMin = Track(bytes, a + 0x0F8, trackBase, 4);
        emitter.LifeMax = Track(bytes, a + 0x110, trackBase, 4);
        emitter.VelocityA = Track(bytes, a + 0x128, trackBase, 6);
        emitter.VelocityB = Track(bytes, a + 0x140, trackBase, 6);
        emitter.SpeedMin = Track(bytes, a + 0x158, trackBase, 2);
        emitter.SpeedMax = Track(bytes, a + 0x170, trackBase, 2);
        emitter.Acceleration = Track(bytes, a + 0x188, trackBase, 6);
        emitter.SpreadMin = Track(bytes, a + 0x1A0, trackBase, 2);
        emitter.Spread = Track(bytes, a + 0x1B8, trackBase, 2);
        emitter.VelocityAScale = Track(bytes, a + 0x1D0, trackBase, 2);
        emitter.RadialSpreadMin = Track(bytes, a + 0x1E8, trackBase, 2);
        emitter.RadialSpread = Track(bytes, a + 0x200, trackBase, 2);
        emitter.RadialScale = Track(bytes, a + 0x218, trackBase, 2);
        emitter.SizeRandomMin = Track(bytes, a + 0x230, trackBase, 2);
        emitter.SizeRandomMax = Track(bytes, a + 0x248, trackBase, 2);
        emitter.MaxParticles = Track(bytes, a + 0x260, trackBase, 4);
        emitter.WorldOffset = Track(bytes, a + 0x278, trackBase, 12);
        emitter.Alpha = Curve(bytes, a, trackBase, 0x400, 1);
        emitter.Size = Curve(bytes, a, trackBase, 0x4F8, 2);
        emitter.Speed = Curve(bytes, a, trackBase, 0x5F0, 2);
        emitter.Colour = ReadColour(bytes, a, trackBase);
        emitter.Flags = U32(bytes, a + 0xEC8);
        emitter.KillFlags = U32(bytes, a + 0x850);
        emitter.KillTolerance = F32(bytes, a + 0x854);
        emitter.RotationMin = F32(bytes, a + 0x808);
        emitter.RotationMax = F32(bytes, a + 0x80C);
        emitter.SpinMin = F32(bytes, a + 0x810);
        emitter.SpinMax = F32(bytes, a + 0x814);
        emitter.SpinAccelerationMin = F32(bytes, a + 0x818);
        emitter.SpinAccelerationMax = F32(bytes, a + 0x81C);
        emitter.TextureIndex = U16(bytes, a + 0x978);
        emitter.FlipU = U16(bytes, a + 0x980) == 1;
        emitter.FlipV = U16(bytes, a + 0x982) == 1;
        emitter.FadeMin = F32(bytes, a + 0x988);
        emitter.FadeMax = F32(bytes, a + 0x98C);
        emitter.TextureIndex1 = U16(bytes, a + 0x994);
        emitter.TextureIndex2 = U16(bytes, a + 0x996);
        emitter.TextureIndex4 = U16(bytes, a + 0x99A);
        emitter.FrameOffsetMin = (int)U32(bytes, a + 0xE70);
        emitter.FrameOffsetMax = (int)U32(bytes, a + 0xE74);
        emitter.FrameTimeMs = Math.Max(BinaryPrimitives.ReadInt32LittleEndian(bytes.AsSpan(a + 0xE78)), 1);
        emitter.UvScaleX = F32(bytes, a + 0xE7C);
        emitter.UvScaleY = F32(bytes, a + 0xE80);
        emitter.ColumnShift = Math.Clamp((int)U32(bytes, a + 0xE84), 0, 16);
        emitter.FrameCount = BinaryPrimitives.ReadInt32LittleEndian(bytes.AsSpan(a + 0xE8C));
        emitter.HoldMinMs = (int)U32(bytes, a + 0xEA4);
        emitter.HoldMaxMs = (int)U32(bytes, a + 0xEA8);
        emitter.MoveMinMs = (int)U32(bytes, a + 0xEAC);
        emitter.MoveMaxMs = (int)U32(bytes, a + 0xEB0);
        return emitter;
    }

    private static M3RawTrack[] ReadShape(byte[] bytes, int block, long trackBase, int shape)
    {
        (int slot, int count) = shape switch
        {
            ShapeLine => (40, 1),
            ShapeRectangle => (56, 2),
            ShapeBox => (64, 3),
            ShapeRing => (72, 4),
            ShapeSphere => (80, 2),
            >= 9 and <= 12 => (96, 6),
            _ => (0, 0),
        };

        if (count == 0)
        {
            return Array.Empty<M3RawTrack>();
        }

        ulong offset = BinaryPrimitives.ReadUInt64LittleEndian(bytes.AsSpan(block + slot));
        if (offset == ulong.MaxValue)
        {
            return Array.Empty<M3RawTrack>();
        }

        long start = trackBase + (long)offset;
        long dataBase = start + 8 + 24L * count;
        if (start < 0 || dataBase > bytes.Length)
        {
            return Array.Empty<M3RawTrack>();
        }

        var tracks = new M3RawTrack[count];
        for (int i = 0; i < count; i++)
        {
            tracks[i] = Track(bytes, (int)(start + 24 * i), dataBase, 2);
        }

        return tracks;
    }

    private static M3ParticleCurve Curve(byte[] bytes, int block, long trackBase, int header, int valueSize)
    {
        int count = Math.Clamp(U16(bytes, block + header), 0, M3ParticleCurve.MaxKeys);
        var ages = new M3RawTrack[count];
        var values = new M3RawTrack[count];
        for (int i = 0; i < count; i++)
        {
            values[i] = Track(bytes, block + header + 8 + 24 * i, trackBase, valueSize);
            ages[i] = Track(bytes, block + header + 128 + 24 * i, trackBase, 1);
        }

        return new M3ParticleCurve(count, ages, values, valueSize);
    }

    private static M3ParticleCurve[] ReadColour(byte[] bytes, int block, long trackBase)
    {
        int count = Math.Clamp(U16(bytes, block + 0x294), 0, M3ParticleCurve.MaxKeys);
        var variants = new M3ParticleCurve[2];
        for (int v = 0; v < 2; v++)
        {
            var ages = new M3RawTrack[count];
            var values = new M3RawTrack[count];
            for (int k = 0; k < count; k++)
            {
                values[k] = Track(bytes, block + 0x298 + 24 * k + 120 * v, trackBase, 4);
                ages[k] = Track(bytes, block + 0x388 + 24 * k, trackBase, 1);
            }

            variants[v] = new M3ParticleCurve(count, ages, values, 4);
        }

        return variants;
    }

    private static M3RawTrack Track(byte[] bytes, int track, long dataBase, int valueSize)
    {
        if (track < 0 || track + 24 > bytes.Length)
        {
            return M3RawTrack.Empty;
        }

        int count = (int)U32(bytes, track);
        if (count <= 0 || count > 4096)
        {
            return M3RawTrack.Empty;
        }

        long keys = dataBase + (long)BinaryPrimitives.ReadUInt64LittleEndian(bytes.AsSpan(track + 8));
        long values = dataBase + (long)BinaryPrimitives.ReadUInt64LittleEndian(bytes.AsSpan(track + 16));
        if (keys < 0 || values < 0 || keys + 4L * count > bytes.Length || values + (long)valueSize * count > bytes.Length)
        {
            return M3RawTrack.Empty;
        }

        var times = new uint[count];
        for (int i = 0; i < count; i++)
        {
            times[i] = BinaryPrimitives.ReadUInt32LittleEndian(bytes.AsSpan((int)keys + 4 * i));
        }

        return new M3RawTrack(times, bytes.AsSpan((int)values, count * valueSize).ToArray(), valueSize);
    }

    private static int U16(byte[] bytes, int offset) => BinaryPrimitives.ReadUInt16LittleEndian(bytes.AsSpan(offset));

    private static uint U32(byte[] bytes, int offset) => BinaryPrimitives.ReadUInt32LittleEndian(bytes.AsSpan(offset));

    private static float F32(byte[] bytes, int offset) => BinaryPrimitives.ReadSingleLittleEndian(bytes.AsSpan(offset));
}
