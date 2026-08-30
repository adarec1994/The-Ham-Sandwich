using System;

namespace WildStar.Sky;

public readonly struct SkyTrack
{
    public static readonly SkyTrack Empty = new(Array.Empty<uint>(), Array.Empty<byte>(), 0);

    public SkyTrack(uint[] keys, byte[] values, int valueSize)
    {
        Keys = keys;
        Values = values;
        ValueSize = valueSize;
    }

    public uint[] Keys { get; }

    public byte[] Values { get; }

    public int ValueSize { get; }

    public int Count => Keys.Length;

    public bool HasKeys => Keys.Length > 0;

    public float Float(int index, int offset) =>
        BitConverter.ToSingle(Values, index * ValueSize + offset);

    public uint UInt(int index, int offset) =>
        BitConverter.ToUInt32(Values, index * ValueSize + offset);

    public float[] Floats(int index, int offset, int count)
    {
        var result = new float[count];
        for (int i = 0; i < count; i++)
        {
            result[i] = Float(index, offset + 4 * i);
        }

        return result;
    }

    public bool Sample(uint time, out int index0, out int index1, out float fraction)
    {
        index0 = 0;
        index1 = 0;
        fraction = 0.0f;
        int count = Keys.Length;
        if (count == 0)
        {
            return false;
        }

        if (count == 1)
        {
            return true;
        }

        int lo = 0;
        int hi = count;
        while (lo < hi)
        {
            int mid = lo + ((hi - lo) >> 1);
            if (time >= Keys[mid])
            {
                lo = mid + 1;
            }
            else
            {
                hi = lo + ((hi - lo) >> 1);
            }
        }

        if (lo == 0)
        {
            return true;
        }

        if (lo == count)
        {
            index0 = count - 1;
            index1 = count - 1;
            return true;
        }

        index0 = lo - 1;
        index1 = lo;
        uint span = Keys[lo] - Keys[lo - 1];
        fraction = span == 0 ? 0.0f : (float)(int)(time - Keys[lo - 1]) / span;
        return true;
    }

    public int SampleWrapped(uint time, out int index0, out int index1, out float fraction)
    {
        index0 = 0;
        index1 = 0;
        fraction = 0.0f;
        int count = Keys.Length;
        if (count == 0)
        {
            return 0;
        }

        if (count == 1)
        {
            return 1;
        }

        int passed = 0;
        while (passed < count && Keys[passed] <= time)
        {
            passed++;
        }

        uint now = time;
        int previous;
        if (passed > 0)
        {
            previous = passed - 1;
        }
        else
        {
            previous = count - 1;
            now = time + SkyFile.SecondsPerDay;
        }

        int next = passed == count ? 0 : passed;
        int numerator = (int)(now - Keys[previous]);
        int denominator = (int)(Keys[next] - Keys[previous]);
        if (previous > next)
        {
            denominator += (int)SkyFile.SecondsPerDay;
        }

        index0 = previous;
        index1 = next;
        fraction = denominator == 0 ? 0.0f : (float)numerator / denominator;
        return 2;
    }

    public float SampleFloat(uint time, int offset, float fallback)
    {
        if (!Sample(time, out int i0, out int i1, out float t))
        {
            return fallback;
        }

        float a = Float(i0, offset);
        return i0 == i1 ? a : a + (Float(i1, offset) - a) * t;
    }

    public float[] SampleFloats(uint time, int offset, int count, float[]? fallback)
    {
        if (!Sample(time, out int i0, out int i1, out float t))
        {
            return fallback is null ? new float[count] : (float[])fallback.Clone();
        }

        var result = new float[count];
        for (int i = 0; i < count; i++)
        {
            float a = Float(i0, offset + 4 * i);
            result[i] = i0 == i1 ? a : a + (Float(i1, offset + 4 * i) - a) * t;
        }

        return result;
    }
}
