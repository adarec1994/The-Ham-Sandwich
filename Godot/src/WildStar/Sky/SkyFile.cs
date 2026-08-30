using System;
using System.Buffers.Binary;
using System.Text;

namespace WildStar.Sky;

public sealed class SkyFile
{
    public const uint Magic = 0x58534B59;
    public const uint Version = 16;
    public const int HeaderSize = 0x4C0;
    public const uint SecondsPerDay = 86400;
    public const int BlockCount = 4;
    public const int ColourPairCount = 8;
    public const int CloudBandCount = 6;
    public const int ParticulateLimit = 4;

    public const uint FlagPrimary = 0x1;
    public const uint FlagSecondary = 0x2;
    public const uint FlagUntintedEnvironmentMap = 0x4;

    public const int ModelKindTimeOfDayAnimation = 0x2;
    public const int TimeOfDaySequence = 150;
    public const int GlareAttachment = 9;
    public const int SunAttachment = 21;

    public const int NoSunModel = -1;

    private SkyFile(uint flags, uint sunModel, float blendScalar, uint reserved20,
                    string sourcePath, SkyLightBlock[] blocks, SkyTrack lightingColourA,
                    SkyTrack sun, SkyTrack colour696, SkyTrack fog, SkyModel[] models,
                    SkyTrack postProcess, SkyTrack cloudSets, SkyTrack cloudAlpha,
                    string[] particulates, string environmentMap, SkyTrack glareColour,
                    string glareModelA, string glareModelB, SkyTrack scalar920,
                    SkyTrack nightAmbient, SkyTrack lightingColourB, SkyTrack[] colourPairs,
                    SkySoundEvent[] soundEvents, string colourLut)
    {
        Flags = flags;
        SunModel = sunModel;
        BlendScalar = blendScalar;
        Reserved20 = reserved20;
        SourcePath = sourcePath;
        Blocks = blocks;
        LightingColourA = lightingColourA;
        Sun = sun;
        Colour696 = colour696;
        Fog = fog;
        Models = models;
        PostProcess = postProcess;
        CloudSets = cloudSets;
        CloudAlpha = cloudAlpha;
        Particulates = particulates;
        EnvironmentMap = environmentMap;
        GlareColour = glareColour;
        GlareModelA = glareModelA;
        GlareModelB = glareModelB;
        Scalar920 = scalar920;
        NightAmbient = nightAmbient;
        LightingColourB = lightingColourB;
        ColourPairs = colourPairs;
        SoundEvents = soundEvents;
        ColourLut = colourLut;
    }

    public uint Flags { get; }

    public uint SunModel { get; }

    public float BlendScalar { get; }

    public uint Reserved20 { get; }

    public string SourcePath { get; }

    public SkyLightBlock[] Blocks { get; }

    public SkyTrack LightingColourA { get; }

    public SkyTrack Sun { get; }

    public SkyTrack Colour696 { get; }

    public SkyTrack Fog { get; }

    public SkyModel[] Models { get; }

    public SkyTrack PostProcess { get; }

    public SkyTrack CloudSets { get; }

    public SkyTrack CloudAlpha { get; }

    public string[] Particulates { get; }

    public string EnvironmentMap { get; }

    public SkyTrack GlareColour { get; }

    public string GlareModelA { get; }

    public string GlareModelB { get; }

    public SkyTrack Scalar920 { get; }

    public SkyTrack NightAmbient { get; }

    public SkyTrack LightingColourB { get; }

    public SkyTrack[] ColourPairs { get; }

    public SkySoundEvent[] SoundEvents { get; }

    public string ColourLut { get; }

    public bool IsPrimary => (Flags & FlagPrimary) != 0;

    public bool IsSunBearing => (Flags & (FlagPrimary | FlagSecondary)) != 0;

    public bool UntintedEnvironmentMap => (Flags & FlagUntintedEnvironmentMap) != 0;

    public int SunModelIndex =>
        SunModel != 0xFFFFFFFF && SunModel < (uint)Models.Length ? (int)SunModel : NoSunModel;

    public SkyLightBlock MainLighting => Blocks[0];

    public SkyLightBlock SkyBlock => Blocks[1];

    public SkyLightBlock FogBlock => Blocks[2];

    public SkyLightBlock CloudBlock => Blocks[3];

    public const int LightingBlockIndex = 0;
    public const int SkyBlockIndex = 1;
    public const int FogBlockIndex = 2;
    public const int CloudBlockIndex = 3;

    public string Name
    {
        get
        {
            int slash = SourcePath.LastIndexOfAny(new[] { '\\', '/' });
            string leaf = slash >= 0 ? SourcePath[(slash + 1)..] : SourcePath;
            int dot = leaf.LastIndexOf('.');
            return dot > 0 ? leaf[..dot] : leaf;
        }
    }

    public static bool TryParse(byte[] bytes, out SkyFile sky, out string error)
    {
        sky = null!;

        if (bytes.Length < HeaderSize)
        {
            error = "shorter than a sky header";
            return false;
        }

        if (BinaryPrimitives.ReadUInt32LittleEndian(bytes) != Magic)
        {
            error = "missing YKSX signature";
            return false;
        }

        uint version = BinaryPrimitives.ReadUInt32LittleEndian(bytes.AsSpan(4));
        if (version != Version)
        {
            error = "unsupported sky version " + version;
            return false;
        }

        try
        {
            var reader = new Reader(bytes);
            sky = reader.Parse();
            error = string.Empty;
            return true;
        }
        catch (SkyFormatException exception)
        {
            error = exception.Message;
            return false;
        }
    }

    private sealed class SkyFormatException : Exception
    {
        public SkyFormatException(string message) : base(message)
        {
        }
    }

    private sealed class Reader
    {
        private readonly byte[] _bytes;

        public Reader(byte[] bytes)
        {
            _bytes = bytes;
        }

        public SkyFile Parse()
        {
            byte[] b = _bytes;
            uint flags = U32(8);
            uint sunModel = U32(12);
            float blendScalar = BitConverter.ToSingle(b, 16);
            uint reserved20 = U32(20);

            string sourcePath = WideString(24, HeaderSize);

            var blocks = new SkyLightBlock[BlockCount];
            for (int i = 0; i < BlockCount; i++)
            {
                blocks[i] = Block(40 + SkyLightBlock.Size * i);
            }

            SkyTrack lightingColourA = Track(648, 16, HeaderSize);
            SkyTrack sun = Track(672, SkyDirectionalLight.ValueSize, HeaderSize);
            SkyTrack colour696 = Track(696, 16, HeaderSize);
            SkyTrack fog = Track(720, SkyFog.ValueSize, HeaderSize);
            SkyModel[] models = Models(744);
            SkyTrack postProcess = Track(760, SkyPostProcess.ValueSize, HeaderSize);
            SkyTrack cloudSets = Track(784, 4, HeaderSize);
            SkyTrack cloudAlpha = Track(808, 4, HeaderSize);
            string[] particulates = StringList(832);
            string environmentMap = WideString(848, HeaderSize);
            SkyTrack glareColour = Track(864, 16, HeaderSize);
            string glareModelA = WideString(888, HeaderSize);
            string glareModelB = WideString(904, HeaderSize);
            SkyTrack scalar920 = Track(920, 4, HeaderSize);
            SkyTrack nightAmbient = Track(944, 16, HeaderSize);
            SkyTrack lightingColourB = Track(968, 16, HeaderSize);

            var colourPairs = new SkyTrack[ColourPairCount];
            for (int i = 0; i < ColourPairCount; i++)
            {
                colourPairs[i] = Track(992 + 24 * i, SkyColourPair.ValueSize, HeaderSize);
            }

            SkySoundEvent[] soundEvents = SoundEvents(1184);
            string colourLut = WideString(1200, HeaderSize);

            return new SkyFile(flags, sunModel, blendScalar, reserved20, sourcePath, blocks,
                               lightingColourA, sun, colour696, fog, models, postProcess,
                               cloudSets, cloudAlpha, particulates, environmentMap, glareColour,
                               glareModelA, glareModelB, scalar920, nightAmbient,
                               lightingColourB, colourPairs, soundEvents, colourLut);
        }

        private uint U32(int at) => BinaryPrimitives.ReadUInt32LittleEndian(_bytes.AsSpan(at));

        private ulong U64(int at) => BinaryPrimitives.ReadUInt64LittleEndian(_bytes.AsSpan(at));

        private int Resolve(ulong offset, int baseOffset, long size, string what)
        {
            ulong start = (ulong)baseOffset + offset;
            if (start > (ulong)_bytes.Length || (long)start + size > _bytes.Length)
            {
                throw new SkyFormatException(what + " runs past the end of the file");
            }

            return (int)start;
        }

        private static int Align16(int value) => (value + 15) & ~15;

        private int Array(int at, int elementSize, int baseOffset, string what, out int count)
        {
            count = (int)U32(at);
            ulong offset = U64(at + 8);
            int start = Resolve(offset, baseOffset, (long)elementSize * count, what);
            return count == 0 ? -1 : start;
        }

        private SkyTrack Track(int at, int valueSize, int baseOffset)
        {
            int count = (int)U32(at);
            int keys = Resolve(U64(at + 8), baseOffset, 4L * count, "track keys at " + at);
            int values = Resolve(U64(at + 16), baseOffset, (long)valueSize * count, "track values at " + at);
            if (count == 0)
            {
                return SkyTrack.Empty;
            }

            var keyArray = new uint[count];
            for (int i = 0; i < count; i++)
            {
                keyArray[i] = U32(keys + 4 * i);
            }

            var valueArray = new byte[valueSize * count];
            System.Array.Copy(_bytes, values, valueArray, 0, valueArray.Length);
            return new SkyTrack(keyArray, valueArray, valueSize);
        }

        private string WideString(int at, int baseOffset)
        {
            int start = Array(at, 2, baseOffset, "string at " + at, out int count);
            if (count == 0)
            {
                return string.Empty;
            }

            string text = Encoding.Unicode.GetString(_bytes, start, 2 * count);
            int nul = text.IndexOf('\0');
            return nul >= 0 ? text[..nul] : text;
        }

        private SkyTrack[] TrackArray(int at, int valueSize, string what)
        {
            int start = Array(at, 24, HeaderSize, what, out int count);
            var tracks = new SkyTrack[count];
            if (count == 0)
            {
                return tracks;
            }

            int nested = start + Align16(24 * count);
            for (int i = 0; i < count; i++)
            {
                tracks[i] = Track(start + 24 * i, valueSize, nested);
            }

            return tracks;
        }

        private SkyLightBlock Block(int at)
        {
            uint flag = U32(at);
            uint reserved = U32(at + 4);
            SkyTrack[] ambient = TrackArray(at + 8, 16, "ambient lights");
            SkyTrack[] directional = TrackArray(at + 24, SkyDirectionalLight.ValueSize, "directional lights");
            SkyTrack[] hemisphere = TrackArray(at + 40, SkyHemisphereLight.ValueSize, "hemisphere lights");
            SkyTrack[] cone = TrackArray(at + 56, SkyConeLight.ValueSize, "cone lights");
            SkyTrack[] sphere = TrackArray(at + 72, SkySphereLight.ValueSize, "sphere lights");
            SkyTrack[] sh = TrackArray(at + 88, SkyLightBlock.SphericalHarmonicSize, "sh lights");
            SkyTrack baseSh = Track(at + 104, SkyLightBlock.SphericalHarmonicSize, HeaderSize);
            SkyTrack gradient = Track(at + 128, SkyGradient.ValueSize, HeaderSize);
            return new SkyLightBlock(flag, reserved, ambient, directional, hemisphere, cone,
                                     sphere, sh, baseSh, gradient);
        }

        private SkyModel[] Models(int at)
        {
            int start = Array(at, SkyModel.RecordSize, HeaderSize, "model records", out int count);
            var models = new SkyModel[count];
            if (count == 0)
            {
                return models;
            }

            int nested = start + Align16(SkyModel.RecordSize * count);
            for (int i = 0; i < count; i++)
            {
                int record = start + SkyModel.RecordSize * i;
                ushort sortOrder = BinaryPrimitives.ReadUInt16LittleEndian(_bytes.AsSpan(record));
                ushort kind = BinaryPrimitives.ReadUInt16LittleEndian(_bytes.AsSpan(record + 2));
                uint reserved = U32(record + 4);
                string path = WideString(record + 8, nested);
                SkyTrack track = Track(record + 24, SkyModel.ValueSize, nested);
                models[i] = new SkyModel(sortOrder, kind, reserved, path, track);
            }

            return models;
        }

        private string[] StringList(int at)
        {
            int start = Array(at, 16, HeaderSize, "string list", out int count);
            var strings = new string[count];
            if (count == 0)
            {
                return strings;
            }

            int nested = start + Align16(16 * count);
            for (int i = 0; i < count; i++)
            {
                strings[i] = WideString(start + 16 * i, nested);
            }

            return strings;
        }

        private SkySoundEvent[] SoundEvents(int at)
        {
            int start = Array(at, SkySoundEvent.RecordSize, HeaderSize, "sound events", out int count);
            var events = new SkySoundEvent[count];
            if (count == 0)
            {
                return events;
            }

            int nested = start + Align16(SkySoundEvent.RecordSize * count);
            for (int i = 0; i < count; i++)
            {
                int record = start + SkySoundEvent.RecordSize * i;
                int n = (int)U32(record);
                uint reserved = U32(record + 4);
                int keys = Resolve(U64(record + 8), nested, 4L * n, "sound event keys");
                int values = Resolve(U64(record + 16), nested, 8L * n, "sound event values");
                var keyArray = new uint[n];
                var ids = new uint[n];
                var parameters = new uint[n];
                for (int j = 0; j < n; j++)
                {
                    keyArray[j] = U32(keys + 4 * j);
                    parameters[j] = U32(values + 8 * j);
                    ids[j] = U32(values + 8 * j + 4);
                }

                events[i] = new SkySoundEvent(reserved, keyArray, ids, parameters);
            }

            return events;
        }
    }
}
