using System;

namespace WildStar.Model;

public sealed class M3SequenceVariant
{
    public M3SequenceVariant(int sequenceId, int field02, int field06, uint start, uint end,
                             float field10, uint selectLow, uint selectHigh)
    {
        SequenceId = sequenceId;
        Field02 = field02;
        Field06 = field06;
        Start = start;
        End = end;
        Field10 = field10;
        SelectLow = selectLow;
        SelectHigh = selectHigh;
    }

    public int SequenceId { get; }

    public int Field02 { get; }

    public int Field06 { get; }

    public uint Start { get; }

    public uint End { get; }

    public uint Duration => End > Start ? End - Start : 0;

    public float Field10 { get; }

    public uint SelectLow { get; }

    public uint SelectHigh { get; }

    public bool AlwaysEligible => SelectLow == SelectHigh;

    public bool Accepts(uint selector)
    {
        if (AlwaysEligible)
        {
            return true;
        }

        return SelectLow < SelectHigh
            ? selector >= SelectLow && selector < SelectHigh
            : selector < SelectHigh || selector >= SelectLow;
    }
}
