using System;
using System.IO;

namespace WildStar.Audio.Vorbis;

internal static class WwiseVorbisRebuilder
{
    private static readonly byte[] VorbisTag = { (byte)'v', (byte)'o', (byte)'r', (byte)'b', (byte)'i', (byte)'s' };
    private static readonly byte[] Vendor = { (byte)'w', (byte)'w', (byte)'2', (byte)'o', (byte)'g', (byte)'g' };

    public static byte[] ToOgg(WemFile wem)
    {
        Layout layout = ReadLayout(wem);
        WwiseCodebooks codebooks = WwiseCodebooks.Default;
        var ogg = new OggPageWriter();

        WriteIdentification(ogg, wem, layout);
        WriteComment(ogg);

        bool[] modeBlockflag = WriteSetup(ogg, wem, layout, codebooks, out int modeBits);

        WriteAudio(ogg, wem, layout, modeBlockflag, modeBits);

        return ogg.ToArray();
    }

    private static void WriteIdentification(OggPageWriter ogg, WemFile wem, Layout layout)
    {
        ogg.WriteBits(1, 8);
        ogg.WriteBytes(VorbisTag);
        ogg.WriteBits(0, 32);
        ogg.WriteBits((uint)wem.Channels, 8);
        ogg.WriteBits((uint)wem.SampleRate, 32);
        ogg.WriteBits(0, 32);
        ogg.WriteBits((uint)(wem.AverageBytesPerSecond * 8), 32);
        ogg.WriteBits(0, 32);
        ogg.WriteBits(layout.BlockSize0Pow, 4);
        ogg.WriteBits(layout.BlockSize1Pow, 4);
        ogg.WriteBits(1, 1);
        ogg.FlushPage(false, true);
    }

    private static void WriteComment(OggPageWriter ogg)
    {
        ogg.WriteBits(3, 8);
        ogg.WriteBytes(VorbisTag);
        ogg.WriteBits((uint)Vendor.Length, 32);
        ogg.WriteBytes(Vendor);
        ogg.WriteBits(0, 32);
        ogg.WriteBits(1, 1);
        ogg.FlushPage();
    }

    private static bool[] WriteSetup(OggPageWriter ogg, WemFile wem, Layout layout,
        WwiseCodebooks codebooks, out int modeBits)
    {
        ogg.WriteBits(5, 8);
        ogg.WriteBytes(VorbisTag);

        int setupOffset = wem.DataOffset + layout.SetupPacketOffset;
        int setupSize = wem.ReadUInt16(setupOffset);
        int setupData = setupOffset + (layout.NoGranule ? 2 : 6);

        if (setupData + setupSize > wem.Bytes.Length)
        {
            throw new InvalidDataException("setup packet runs past the end of the file");
        }

        var setup = new VorbisBitReader(wem.Bytes.AsMemory(setupData, setupSize));

        uint codebookCount = setup.Read(8) + 1;
        ogg.WriteBits(codebookCount - 1, 8);

        for (uint i = 0; i < codebookCount; i++)
        {
            RebuildCodebook(new VorbisBitReader(codebooks.Get((int)setup.Read(10))), ogg);
        }

        ogg.WriteBits(0, 6);
        ogg.WriteBits(0, 16);

        WriteFloors(ogg, setup);
        WriteResidues(ogg, setup);
        WriteMappings(ogg, setup, wem.Channels);

        bool[] modeBlockflag = WriteModes(ogg, setup, out modeBits);

        ogg.WriteBits(1, 1);
        ogg.FlushPage();

        return modeBlockflag;
    }

    private static void WriteFloors(OggPageWriter ogg, VorbisBitReader setup)
    {
        uint floorCount = setup.Read(6) + 1;
        ogg.WriteBits(floorCount - 1, 6);

        for (uint i = 0; i < floorCount; i++)
        {
            ogg.WriteBits(1, 16);

            uint partitions = setup.Read(5);
            ogg.WriteBits(partitions, 5);

            var partitionClasses = new uint[partitions];
            uint maxClass = 0;

            for (uint j = 0; j < partitions; j++)
            {
                uint partitionClass = setup.Read(4);
                ogg.WriteBits(partitionClass, 4);
                partitionClasses[j] = partitionClass;
                maxClass = Math.Max(maxClass, partitionClass);
            }

            var classDimensions = new uint[maxClass + 1];

            for (uint j = 0; j <= maxClass; j++)
            {
                uint dimensions = setup.Read(3);
                ogg.WriteBits(dimensions, 3);
                classDimensions[j] = dimensions + 1;

                uint subclasses = setup.Read(2);
                ogg.WriteBits(subclasses, 2);

                if (subclasses != 0)
                {
                    ogg.WriteBits(setup.Read(8), 8);
                }

                for (uint k = 0; k < 1u << (int)subclasses; k++)
                {
                    ogg.WriteBits(setup.Read(8), 8);
                }
            }

            ogg.WriteBits(setup.Read(2), 2);
            int rangeBits = (int)setup.Read(4);
            ogg.WriteBits((uint)rangeBits, 4);

            for (uint j = 0; j < partitions; j++)
            {
                for (uint k = 0; k < classDimensions[partitionClasses[j]]; k++)
                {
                    ogg.WriteBits(setup.Read(rangeBits), rangeBits);
                }
            }
        }
    }

    private static void WriteResidues(OggPageWriter ogg, VorbisBitReader setup)
    {
        uint residueCount = setup.Read(6) + 1;
        ogg.WriteBits(residueCount - 1, 6);

        for (uint i = 0; i < residueCount; i++)
        {
            ogg.WriteBits(setup.Read(2), 16);
            ogg.WriteBits(setup.Read(24), 24);
            ogg.WriteBits(setup.Read(24), 24);
            ogg.WriteBits(setup.Read(24), 24);

            uint classifications = setup.Read(6) + 1;
            ogg.WriteBits(classifications - 1, 6);
            ogg.WriteBits(setup.Read(8), 8);

            var cascade = new uint[classifications];

            for (uint j = 0; j < classifications; j++)
            {
                uint lowBits = setup.Read(3);
                ogg.WriteBits(lowBits, 3);

                uint flag = setup.Read(1);
                ogg.WriteBits(flag, 1);

                uint highBits = 0;
                if (flag != 0)
                {
                    highBits = setup.Read(5);
                    ogg.WriteBits(highBits, 5);
                }

                cascade[j] = highBits * 8 + lowBits;
            }

            for (uint j = 0; j < classifications; j++)
            {
                for (int k = 0; k < 8; k++)
                {
                    if ((cascade[j] & (1u << k)) != 0)
                    {
                        ogg.WriteBits(setup.Read(8), 8);
                    }
                }
            }
        }
    }

    private static void WriteMappings(OggPageWriter ogg, VorbisBitReader setup, int channels)
    {
        uint mappingCount = setup.Read(6) + 1;
        ogg.WriteBits(mappingCount - 1, 6);

        for (uint i = 0; i < mappingCount; i++)
        {
            ogg.WriteBits(0, 16);

            uint submapsFlag = setup.Read(1);
            ogg.WriteBits(submapsFlag, 1);

            uint submaps = 1;
            if (submapsFlag != 0)
            {
                submaps = setup.Read(4) + 1;
                ogg.WriteBits(submaps - 1, 4);
            }

            uint squarePolar = setup.Read(1);
            ogg.WriteBits(squarePolar, 1);

            if (squarePolar != 0)
            {
                uint couplingSteps = setup.Read(8) + 1;
                ogg.WriteBits(couplingSteps - 1, 8);

                int channelBits = ILog((uint)(channels - 1));
                for (uint j = 0; j < couplingSteps; j++)
                {
                    ogg.WriteBits(setup.Read(channelBits), channelBits);
                    ogg.WriteBits(setup.Read(channelBits), channelBits);
                }
            }

            ogg.WriteBits(setup.Read(2), 2);

            if (submaps > 1)
            {
                for (int j = 0; j < channels; j++)
                {
                    ogg.WriteBits(setup.Read(4), 4);
                }
            }

            for (uint j = 0; j < submaps; j++)
            {
                ogg.WriteBits(setup.Read(8), 8);
                ogg.WriteBits(setup.Read(8), 8);
                ogg.WriteBits(setup.Read(8), 8);
            }
        }
    }

    private static bool[] WriteModes(OggPageWriter ogg, VorbisBitReader setup, out int modeBits)
    {
        uint modeCount = setup.Read(6) + 1;
        ogg.WriteBits(modeCount - 1, 6);

        var modeBlockflag = new bool[modeCount];
        modeBits = ILog(modeCount - 1);

        for (uint i = 0; i < modeCount; i++)
        {
            uint blockflag = setup.Read(1);
            ogg.WriteBits(blockflag, 1);
            modeBlockflag[i] = blockflag != 0;

            ogg.WriteBits(0, 16);
            ogg.WriteBits(0, 16);
            ogg.WriteBits(setup.Read(8), 8);
        }

        return modeBlockflag;
    }

    private static void WriteAudio(OggPageWriter ogg, WemFile wem, Layout layout,
        bool[] modeBlockflag, int modeBits)
    {
        int offset = wem.DataOffset + layout.FirstAudioPacketOffset;
        int end = Math.Min(wem.DataOffset + wem.DataSize, wem.Bytes.Length);
        int headerSize = layout.NoGranule ? 2 : 6;

        int shortBlock = 1 << layout.BlockSize0Pow;
        int longBlock = 1 << layout.BlockSize1Pow;

        bool previousBlockflag = false;
        int previousBlock = 0;
        ulong granule = 0;

        while (offset + headerSize <= end)
        {
            int packetSize = wem.ReadUInt16(offset);
            uint packetGranule = layout.NoGranule ? 0 : wem.ReadUInt32(offset + 2);

            if (offset + headerSize + packetSize > end || packetSize == 0)
            {
                break;
            }

            int packet = offset + headerSize;
            bool last = packet + packetSize >= end;

            uint mode = ReadMode(wem, packet, packetSize, layout.ModPackets, modeBits);
            int block = mode < modeBlockflag.Length && modeBlockflag[mode] ? longBlock : shortBlock;

            if (previousBlock != 0)
            {
                granule += (ulong)(previousBlock + block) / 4;
            }

            previousBlock = block;

            if (layout.NoGranule)
            {
                ogg.SetGranule(last && layout.SampleCount != 0
                    ? Math.Min(granule, layout.SampleCount)
                    : granule);
            }
            else
            {
                ogg.SetGranule(packetGranule == 0xFFFFFFFF ? 0 : packetGranule);
            }

            if (layout.ModPackets && modeBits > 0)
            {
                var input = new VorbisBitReader(wem.Bytes.AsMemory(packet, packetSize));

                ogg.WriteBits(0, 1);
                ogg.WriteBits(input.Read(modeBits), modeBits);

                uint remainder = input.Read(8 - modeBits);

                if (mode < modeBlockflag.Length && modeBlockflag[mode])
                {
                    bool nextBlockflag = false;
                    int next = packet + packetSize;

                    if (next + headerSize <= end)
                    {
                        int nextSize = wem.ReadUInt16(next);
                        if (nextSize > 0 && next + headerSize + nextSize <= end)
                        {
                            uint nextMode = ReadMode(wem, next + headerSize, nextSize, true, modeBits);
                            if (nextMode < modeBlockflag.Length)
                            {
                                nextBlockflag = modeBlockflag[nextMode];
                            }
                        }
                    }

                    ogg.WriteBits(previousBlockflag ? 1u : 0u, 1);
                    ogg.WriteBits(nextBlockflag ? 1u : 0u, 1);
                }

                if (mode < modeBlockflag.Length)
                {
                    previousBlockflag = modeBlockflag[mode];
                }

                ogg.WriteBits(remainder, 8 - modeBits);
                ogg.WriteBytes(wem.Bytes.AsSpan(packet + 1, packetSize - 1));
            }
            else
            {
                ogg.WriteBytes(wem.Bytes.AsSpan(packet, packetSize));
            }

            offset = packet + packetSize;
            ogg.FlushPage(false, false, last);
        }
    }

    private static uint ReadMode(WemFile wem, int packet, int packetSize, bool modPackets,
        int modeBits)
    {
        if (modeBits == 0)
        {
            return 0;
        }

        var reader = new VorbisBitReader(wem.Bytes.AsMemory(packet, packetSize));

        if (!modPackets)
        {
            reader.Read(1);
        }

        return reader.Read(modeBits);
    }

    private static void RebuildCodebook(VorbisBitReader input, OggPageWriter ogg)
    {
        ogg.WriteBits(0x564342, 24);

        uint dimensions = input.Read(4);
        ogg.WriteBits(dimensions, 16);

        uint entries = input.Read(14);
        ogg.WriteBits(entries, 24);

        uint ordered = input.Read(1);
        ogg.WriteBits(ordered, 1);

        if (ordered != 0)
        {
            ogg.WriteBits(input.Read(5), 5);

            uint current = 0;
            while (current < entries)
            {
                int bits = ILog(entries - current);
                uint number = input.Read(bits);
                ogg.WriteBits(number, bits);
                current += number;
            }

            if (current > entries)
            {
                throw new InvalidDataException("codebook declares more entries than it has");
            }
        }
        else
        {
            int lengthBits = (int)input.Read(3);
            uint sparse = input.Read(1);
            ogg.WriteBits(sparse, 1);

            if (lengthBits == 0 || lengthBits > 5)
            {
                throw new InvalidDataException("codebook has an impossible codeword length");
            }

            for (uint i = 0; i < entries; i++)
            {
                bool present = true;

                if (sparse != 0)
                {
                    uint flag = input.Read(1);
                    ogg.WriteBits(flag, 1);
                    present = flag != 0;
                }

                if (present)
                {
                    ogg.WriteBits(input.Read(lengthBits), 5);
                }
            }
        }

        uint lookupType = input.Read(1);
        ogg.WriteBits(lookupType, 4);

        if (lookupType == 0)
        {
            return;
        }

        ogg.WriteBits(input.Read(32), 32);
        ogg.WriteBits(input.Read(32), 32);

        int valueLength = (int)input.Read(4);
        ogg.WriteBits((uint)valueLength, 4);
        ogg.WriteBits(input.Read(1), 1);

        uint quantvals = QuantValues(entries, dimensions);
        for (uint i = 0; i < quantvals; i++)
        {
            ogg.WriteBits(input.Read(valueLength + 1), valueLength + 1);
        }
    }

    private static Layout ReadLayout(WemFile wem)
    {
        if (wem.Codec != WemCodec.WwiseVorbis && wem.Codec != WemCodec.WwiseVorbisAlt)
        {
            throw new InvalidDataException("not a Wwise Vorbis stream");
        }

        if (wem.VorbOffset < 0)
        {
            throw new InvalidDataException("no vorb chunk and fmt is not the extended form");
        }

        var layout = new Layout { VorbSize = wem.VorbSize, SampleCount = wem.ReadUInt32(wem.VorbOffset) };
        int vorb = wem.VorbOffset;

        switch (wem.VorbSize)
        {
            case 0x2A:
                layout.NoGranule = true;
                uint signal = wem.ReadUInt32(vorb + 0x04);
                layout.ModPackets = signal != 0x4A && signal != 0x4B && signal != 0x69 && signal != 0x70;
                layout.SetupPacketOffset = (int)wem.ReadUInt32(vorb + 0x10);
                layout.FirstAudioPacketOffset = (int)wem.ReadUInt32(vorb + 0x14);
                layout.BlockSize0Pow = wem.Bytes[vorb + 0x28];
                layout.BlockSize1Pow = wem.Bytes[vorb + 0x29];
                break;

            case 0x32:
            case 0x34:
                layout.SetupPacketOffset = (int)wem.ReadUInt32(vorb + 0x18);
                layout.FirstAudioPacketOffset = (int)wem.ReadUInt32(vorb + 0x1C);
                layout.BlockSize0Pow = wem.Bytes[vorb + 0x30];
                layout.BlockSize1Pow = wem.Bytes[vorb + 0x31];
                break;

            default:
                throw new InvalidDataException("unsupported vorb chunk of " + wem.VorbSize + " bytes");
        }

        return layout;
    }

    private static int ILog(uint value)
    {
        int bits = 0;
        while (value != 0)
        {
            bits++;
            value >>= 1;
        }

        return bits;
    }

    private static uint QuantValues(uint entries, uint dimensions)
    {
        if (dimensions == 0)
        {
            throw new InvalidDataException("codebook has zero dimensions");
        }

        int bits = ILog(entries);
        long values = entries >> (int)((bits - 1) * (dimensions - 1) / dimensions);

        while (true)
        {
            long low = 1;
            long high = 1;

            for (uint i = 0; i < dimensions; i++)
            {
                low *= values;
                high *= values + 1;
            }

            if (low <= entries && high > entries)
            {
                return (uint)values;
            }

            if (low > entries)
            {
                values--;
            }
            else
            {
                values++;
            }
        }
    }

    private sealed class Layout
    {
        public int VorbSize;
        public uint SampleCount;
        public bool NoGranule;
        public bool ModPackets;
        public int SetupPacketOffset;
        public int FirstAudioPacketOffset;
        public byte BlockSize0Pow;
        public byte BlockSize1Pow;
    }
}
