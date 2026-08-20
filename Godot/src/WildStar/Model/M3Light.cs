using System;

namespace WildStar.Model;

public sealed class M3Light
{
    public const int TrackCount = 15;

    public M3Light(byte[] record, M3RawTrack[] tracks)
    {
        Record = record;
        Tracks = tracks;
    }

    public byte[] Record { get; }

    public M3RawTrack[] Tracks { get; }
}
