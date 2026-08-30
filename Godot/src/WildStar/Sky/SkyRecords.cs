using System;

namespace WildStar.Sky;

public readonly struct SkyColour
{
    public static readonly SkyColour Black = new(0.0f, 0.0f, 0.0f, 1.0f);
    public static readonly SkyColour White = new(1.0f, 1.0f, 1.0f, 1.0f);
    public static readonly SkyColour Zero = new(0.0f, 0.0f, 0.0f, 0.0f);

    public SkyColour(float r, float g, float b, float a)
    {
        R = r;
        G = g;
        B = b;
        A = a;
    }

    public float R { get; }

    public float G { get; }

    public float B { get; }

    public float A { get; }

    public static SkyColour FromTrack(SkyTrack track, int index, int offset) =>
        new(track.Float(index, offset), track.Float(index, offset + 4),
            track.Float(index, offset + 8), track.Float(index, offset + 12));

    public static SkyColour Lerp(SkyColour a, SkyColour b, float t) =>
        new(a.R + (b.R - a.R) * t, a.G + (b.G - a.G) * t, a.B + (b.B - a.B) * t,
            a.A + (b.A - a.A) * t);

    public static SkyColour Sample(SkyTrack track, uint time, int offset, SkyColour fallback)
    {
        if (!track.Sample(time, out int i0, out int i1, out float t))
        {
            return fallback;
        }

        SkyColour a = FromTrack(track, i0, offset);
        return i0 == i1 ? a : Lerp(a, FromTrack(track, i1, offset), t);
    }

    public SkyColour Scale(float s) => new(R * s, G * s, B * s, A * s);

    public override string ToString() => $"({R:0.###}, {G:0.###}, {B:0.###}, {A:0.###})";
}

public readonly struct SkyDirection
{
    public SkyDirection(float elevation, float azimuth)
    {
        Elevation = elevation;
        Azimuth = azimuth;
    }

    public float Elevation { get; }

    public float Azimuth { get; }

    public static SkyDirection FromTrack(SkyTrack track, int index) =>
        new(track.Float(index, 0), track.Float(index, 4));

    public static SkyDirection Lerp(SkyDirection a, SkyDirection b, float t) =>
        new(a.Elevation + (b.Elevation - a.Elevation) * t, a.Azimuth + (b.Azimuth - a.Azimuth) * t);
}

public readonly struct SkyDirectionalLight
{
    public const int ValueSize = 32;

    public SkyDirectionalLight(SkyDirection direction, SkyColour colour)
    {
        Direction = direction;
        Colour = colour;
    }

    public SkyDirection Direction { get; }

    public SkyColour Colour { get; }

    public static SkyDirectionalLight Read(SkyTrack track, int index) =>
        new(SkyDirection.FromTrack(track, index), SkyColour.FromTrack(track, index, 16));

    public static SkyDirectionalLight Sample(SkyTrack track, uint time)
    {
        track.Sample(time, out int i0, out int i1, out float t);
        SkyDirectionalLight a = Read(track, i0);
        if (i0 == i1)
        {
            return a;
        }

        SkyDirectionalLight b = Read(track, i1);
        return new SkyDirectionalLight(SkyDirection.Lerp(a.Direction, b.Direction, t),
                                       SkyColour.Lerp(a.Colour, b.Colour, t));
    }
}

public readonly struct SkyHemisphereLight
{
    public const int ValueSize = 48;

    public SkyHemisphereLight(SkyDirection direction, SkyColour skyColour, SkyColour groundColour)
    {
        Direction = direction;
        SkyColour = skyColour;
        GroundColour = groundColour;
    }

    public SkyDirection Direction { get; }

    public SkyColour SkyColour { get; }

    public SkyColour GroundColour { get; }

    public static SkyHemisphereLight Read(SkyTrack track, int index) =>
        new(SkyDirection.FromTrack(track, index), SkyColour.FromTrack(track, index, 16),
            SkyColour.FromTrack(track, index, 32));

    public static SkyHemisphereLight Sample(SkyTrack track, uint time)
    {
        track.Sample(time, out int i0, out int i1, out float t);
        SkyHemisphereLight a = Read(track, i0);
        if (i0 == i1)
        {
            return a;
        }

        SkyHemisphereLight b = Read(track, i1);
        return new SkyHemisphereLight(SkyDirection.Lerp(a.Direction, b.Direction, t),
                                      SkyColour.Lerp(a.SkyColour, b.SkyColour, t),
                                      SkyColour.Lerp(a.GroundColour, b.GroundColour, t));
    }
}

public readonly struct SkyConeLight
{
    public const int ValueSize = 48;

    public SkyConeLight(SkyDirection direction, SkyColour colour, float angle)
    {
        Direction = direction;
        Colour = colour;
        Angle = angle;
    }

    public SkyDirection Direction { get; }

    public SkyColour Colour { get; }

    public float Angle { get; }

    public static SkyConeLight Read(SkyTrack track, int index) =>
        new(SkyDirection.FromTrack(track, index), SkyColour.FromTrack(track, index, 16),
            track.Float(index, 32));

    public static SkyConeLight Sample(SkyTrack track, uint time)
    {
        track.Sample(time, out int i0, out int i1, out float t);
        SkyConeLight a = Read(track, i0);
        if (i0 == i1)
        {
            return a;
        }

        SkyConeLight b = Read(track, i1);
        return new SkyConeLight(SkyDirection.Lerp(a.Direction, b.Direction, t),
                                SkyColour.Lerp(a.Colour, b.Colour, t),
                                a.Angle + (b.Angle - a.Angle) * t);
    }
}

public readonly struct SkySphereLight
{
    public const int ValueSize = 48;

    public SkySphereLight(SkyDirection direction, SkyColour colour, float radius, float distance)
    {
        Direction = direction;
        Colour = colour;
        Radius = radius;
        Distance = distance;
    }

    public SkyDirection Direction { get; }

    public SkyColour Colour { get; }

    public float Radius { get; }

    public float Distance { get; }

    public static SkySphereLight Read(SkyTrack track, int index) =>
        new(SkyDirection.FromTrack(track, index), SkyColour.FromTrack(track, index, 16),
            track.Float(index, 32), track.Float(index, 36));

    public static SkySphereLight Sample(SkyTrack track, uint time)
    {
        track.Sample(time, out int i0, out int i1, out float t);
        SkySphereLight a = Read(track, i0);
        if (i0 == i1)
        {
            return a;
        }

        SkySphereLight b = Read(track, i1);
        return new SkySphereLight(SkyDirection.Lerp(a.Direction, b.Direction, t),
                                  SkyColour.Lerp(a.Colour, b.Colour, t),
                                  a.Radius + (b.Radius - a.Radius) * t,
                                  a.Distance + (b.Distance - a.Distance) * t);
    }
}

public readonly struct SkyGradient
{
    public const int ValueSize = 272;
    public const int BandCount = 16;

    public SkyGradient(float value0, float value1, SkyColour[] bands)
    {
        Value0 = value0;
        Value1 = value1;
        Bands = bands;
    }

    public float Value0 { get; }

    public float Value1 { get; }

    public SkyColour[] Bands { get; }

    public static SkyGradient Read(SkyTrack track, int index)
    {
        var bands = new SkyColour[BandCount];
        for (int i = 0; i < BandCount; i++)
        {
            bands[i] = SkyColour.FromTrack(track, index, 16 + 16 * i);
        }

        return new SkyGradient(track.Float(index, 0), track.Float(index, 4), bands);
    }

    public static SkyGradient Sample(SkyTrack track, uint time)
    {
        track.Sample(time, out int i0, out int i1, out float t);
        SkyGradient a = Read(track, i0);
        if (i0 == i1)
        {
            return a;
        }

        SkyGradient b = Read(track, i1);
        var bands = new SkyColour[BandCount];
        for (int i = 0; i < BandCount; i++)
        {
            bands[i] = SkyColour.Lerp(a.Bands[i], b.Bands[i], t);
        }

        return new SkyGradient(a.Value0 + (b.Value0 - a.Value0) * t,
                               a.Value1 + (b.Value1 - a.Value1) * t, bands);
    }
}

public sealed class SkyLightBlock
{
    public const int Size = 152;
    public const int SphericalHarmonicSize = 108;
    public const int ShCoefficients = 27;

    public SkyLightBlock(uint flag, uint reserved, SkyTrack[] ambientLights,
                         SkyTrack[] directionalLights, SkyTrack[] hemisphereLights,
                         SkyTrack[] coneLights, SkyTrack[] sphereLights, SkyTrack[] shLights,
                         SkyTrack baseSh, SkyTrack gradient)
    {
        Flag = flag;
        Reserved = reserved;
        AmbientLights = ambientLights;
        DirectionalLights = directionalLights;
        HemisphereLights = hemisphereLights;
        ConeLights = coneLights;
        SphereLights = sphereLights;
        ShLights = shLights;
        BaseSh = baseSh;
        Gradient = gradient;
    }

    public uint Flag { get; }

    public uint Reserved { get; }

    public SkyTrack[] AmbientLights { get; }

    public SkyTrack[] DirectionalLights { get; }

    public SkyTrack[] HemisphereLights { get; }

    public SkyTrack[] ConeLights { get; }

    public SkyTrack[] SphereLights { get; }

    public SkyTrack[] ShLights { get; }

    public SkyTrack BaseSh { get; }

    public SkyTrack Gradient { get; }

    public bool HasLights =>
        AmbientLights.Length > 0 || DirectionalLights.Length > 0 || HemisphereLights.Length > 0 ||
        ConeLights.Length > 0 || SphereLights.Length > 0 || ShLights.Length > 0 || BaseSh.HasKeys;

    public bool HasAny => HasLights || Gradient.HasKeys;
}

public readonly struct SkyModelValue
{
    public SkyModelValue(SkyDirection direction, SkyColour tint)
    {
        Direction = direction;
        Tint = tint;
    }

    public SkyDirection Direction { get; }

    public SkyColour Tint { get; }

    public static SkyModelValue Read(SkyTrack track, int index) =>
        new(SkyDirection.FromTrack(track, index), SkyColour.FromTrack(track, index, 16));

    public static SkyModelValue Lerp(SkyModelValue a, SkyModelValue b, float t) =>
        new(SkyDirection.Lerp(a.Direction, b.Direction, t), SkyColour.Lerp(a.Tint, b.Tint, t));
}

public sealed class SkyModel
{
    public const int RecordSize = 48;
    public const int ValueSize = 32;

    public SkyModel(ushort sortOrder, ushort kind, uint reserved, string path, SkyTrack track)
    {
        SortOrder = sortOrder;
        Kind = kind;
        Reserved = reserved;
        Path = path;
        Track = track;
    }

    public ushort SortOrder { get; }

    public ushort Kind { get; }

    public uint Reserved { get; }

    public string Path { get; }

    public SkyTrack Track { get; }

    public bool AnimatesWithTimeOfDay => (Kind & SkyFile.ModelKindTimeOfDayAnimation) != 0;

    public bool TrySample(uint time, out SkyModelValue value)
    {
        int found = Track.SampleWrapped(time, out int i0, out int i1, out float t);
        if (found == 0)
        {
            value = default;
            return false;
        }

        SkyModelValue a = SkyModelValue.Read(Track, i0);
        value = found == 1 ? a : SkyModelValue.Lerp(a, SkyModelValue.Read(Track, i1), t);
        return true;
    }
}

public readonly struct SkyFog
{
    public const int ValueSize = 24;

    public SkyFog(float[] values)
    {
        Values = values;
    }

    public float[] Values { get; }

    public float Start => Values[0];

    public float End => Values[1];

    public float Unused2 => Values[2];

    public float Unused3 => Values[3];

    public float Density => Values[4];

    public float Mode => Values[5];

    public static readonly float[] Defaults = { 50.0f, 700.0f, 0.0f, 0.0f, 0.5f, 0.0f };

    public static readonly SkyColour EmptyColour = new(0.0f, 0.0f, 0.0f, 1.0f);

    public static readonly SkyColour EmptyColourB = new(0.5f, 0.5f, 0.5f, 1.0f);

    public static SkyFog Sample(SkyTrack track, uint time) =>
        new(track.SampleFloats(time, 0, 6, Defaults));
}

public readonly struct SkyPostProcess
{
    public const int ValueSize = 96;
    public const int FloatCount = 24;

    public SkyPostProcess(float[] values)
    {
        Values = values;
    }

    public float[] Values { get; }

    public SkyColour Colour => new(Values[0], Values[1], Values[2], Values[3]);

    public static readonly int[] InterpolatedFields = { 0, 1, 2, 3, 6, 7, 8, 9, 15, 16, 17, 18, 19, 20, 21 };

    public static readonly int[] WorldFields = { 6, 7, 8, 9, 15, 16, 17, 18, 19, 20, 21 };

    public static SkyPostProcess Sample(SkyTrack track, uint time)
    {
        if (!track.Sample(time, out int i0, out int i1, out float t))
        {
            return new SkyPostProcess(new float[FloatCount]);
        }

        float[] a = track.Floats(i0, 0, FloatCount);
        if (i0 == i1)
        {
            return new SkyPostProcess(a);
        }

        float[] b = track.Floats(i1, 0, FloatCount);
        foreach (int field in InterpolatedFields)
        {
            a[field] += (b[field] - a[field]) * t;
        }

        return new SkyPostProcess(a);
    }
}

public sealed class SkySoundEvent
{
    public const int RecordSize = 24;

    public SkySoundEvent(uint reserved, uint[] keys, uint[] soundIds, uint[] parameters)
    {
        Reserved = reserved;
        Keys = keys;
        SoundIds = soundIds;
        Parameters = parameters;
    }

    public uint Reserved { get; }

    public uint[] Keys { get; }

    public uint[] SoundIds { get; }

    public uint[] Parameters { get; }

    public int Count => Keys.Length;
}

public readonly struct SkyColourPair
{
    public const int ValueSize = 32;

    public SkyColourPair(SkyColour first, SkyColour second)
    {
        First = first;
        Second = second;
    }

    public SkyColour First { get; }

    public SkyColour Second { get; }

    public static readonly SkyColourPair Default = new(SkyColour.Black, SkyColour.White);

    public static SkyColourPair Sample(SkyTrack track, uint time)
    {
        if (!track.Sample(time, out int i0, out int i1, out float t))
        {
            return Default;
        }

        var a = new SkyColourPair(SkyColour.FromTrack(track, i0, 0), SkyColour.FromTrack(track, i0, 16));
        if (i0 == i1)
        {
            return a;
        }

        var b = new SkyColourPair(SkyColour.FromTrack(track, i1, 0), SkyColour.FromTrack(track, i1, 16));
        return new SkyColourPair(SkyColour.Lerp(a.First, b.First, t), SkyColour.Lerp(a.Second, b.Second, t));
    }
}
