using System;

namespace WildStar.Model;

public sealed class M3Animation
{
    public const int LoopFlag = 0x01;

    public M3Animation(int sequenceId, int flags, int variation, uint start, uint end,
                       ushort[] related,
                       int selectionWeight, int crossfadeMs, float naturalSpeed)
    {
        SequenceId = sequenceId;
        Flags = flags;
        Variation = variation;
        Start = start;
        End = end;
        Related = related;
        SelectionWeight = selectionWeight;
        CrossfadeMs = crossfadeMs;
        NaturalSpeed = naturalSpeed;
    }

    public int SequenceId { get; }

    public int Flags { get; }

    public int Variation { get; }

    public bool Loops => (Flags & LoopFlag) != 0;

    public int SelectionWeight { get; }

    public int CrossfadeMs { get; }

    public float NaturalSpeed { get; }

    public uint Start { get; }

    public uint End { get; }

    public ushort[] Related { get; }

    public uint Duration => End > Start ? End - Start : 0;

    public float Seconds => Duration / 1000.0f;
}

public sealed class M3Track
{
    public static readonly M3Track Empty = new(Array.Empty<uint>(), Array.Empty<float>(), 1);

    public M3Track(uint[] keys, float[] values, int stride)
    {
        Keys = keys;
        Values = values;
        Stride = stride;
    }

    public uint[] Keys { get; }

    public float[] Values { get; }

    public int Stride { get; }

    public int Count => Keys.Length;

    public bool HasKeys => Keys.Length != 0;

    public float Component(int key, int component) => Values[key * Stride + component];

    public int UpperBound(uint time)
    {
        int low = 0;
        int high = Keys.Length;

        while (low < high)
        {
            int mid = low + ((high - low) / 2);

            if (time >= Keys[mid])
            {
                low = mid + 1;
            }
            else
            {
                high = mid;
            }
        }

        return low;
    }

    public void Sample(uint time, Span<float> destination)
    {
        if (Count == 0)
        {
            destination[..Stride].Clear();
            return;
        }

        int index = UpperBound(time);

        if (index == 0)
        {
            CopyKey(0, destination);
            return;
        }

        if (index >= Count)
        {
            CopyKey(Count - 1, destination);
            return;
        }

        uint before = Keys[index - 1];
        uint after = Keys[index];
        float blend = after > before ? (time - before) / (float)(after - before) : 0.0f;

        for (int c = 0; c < Stride; c++)
        {
            float a = Component(index - 1, c);
            destination[c] = a + (blend * (Component(index, c) - a));
        }
    }

    public void SampleSlerp(uint time, Span<float> destination)
    {
        if (Count == 0)
        {
            destination[..4].Clear();
            return;
        }

        int index = UpperBound(time);

        if (index == 0)
        {
            CopyKey(0, destination);
            return;
        }

        if (index >= Count)
        {
            CopyKey(Count - 1, destination);
            return;
        }

        uint before = Keys[index - 1];
        uint after = Keys[index];
        float blend = after > before ? (time - before) / (float)(after - before) : 0.0f;

        int offA = (index - 1) * Stride;
        int offB = index * Stride;
        ReadOnlySpan<float> a = Values.AsSpan(offA, 4);
        ReadOnlySpan<float> b = Values.AsSpan(offB, 4);

        M3Slerp.Slerp(destination, a, b, blend);
    }

    private void CopyKey(int key, Span<float> destination)
    {
        for (int c = 0; c < Stride; c++)
        {
            destination[c] = Component(key, c);
        }
    }
}
