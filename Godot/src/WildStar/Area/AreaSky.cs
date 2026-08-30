using System;

namespace WildStar.Area;

public static class AreaSky
{
    public const int Quadrants = 4;
    public const int Slots = 4;
    public const int ValuesPerChunk = Quadrants * Slots;
    public const int MaxActive = 4;
    public const uint DefaultSkyId = 1;
    public const float WeightEpsilon = 0.0000099999997f;

    public static int Index(int qx, int qy, int slot) => (qy * 2 + qx) * Slots + slot;

    public static int Sample(ReadOnlySpan<uint> ids, ReadOnlySpan<byte> weights, float fx, float fy,
                             Span<uint> outIds, Span<float> outWeights)
    {
        Span<uint> candidateIds = stackalloc uint[ValuesPerChunk];
        Span<float> candidateWeights = stackalloc float[ValuesPerChunk];
        int count = 0;
        for (int qy = 0; qy < 2; qy++)
        {
            float rowFactor = qy == 0 ? 1.0f - fy : fy;
            for (int qx = 0; qx < 2; qx++)
            {
                float factor = (qx == 0 ? 1.0f - fx : fx) * rowFactor;
                for (int slot = 0; slot < Slots; slot++)
                {
                    int i = Index(qx, qy, slot);
                    uint id = ids[i];
                    byte weight = weights[i];
                    if (id == 0 || weight == 0)
                    {
                        continue;
                    }

                    candidateIds[count] = id;
                    candidateWeights[count] = weight * 0.0039215689f * factor;
                    count++;
                }
            }
        }

        int merged = 0;
        for (int i = 0; i < count; i++)
        {
            uint id = candidateIds[i];
            float weight = candidateWeights[i];
            int found = -1;
            for (int j = 0; j < merged; j++)
            {
                if (candidateIds[j] == id)
                {
                    found = j;
                    break;
                }
            }

            if (found >= 0)
            {
                candidateWeights[found] += weight;
            }
            else
            {
                candidateIds[merged] = id;
                candidateWeights[merged] = weight;
                merged++;
            }
        }

        for (int i = 0; i < merged; i++)
        {
            int best = i;
            for (int j = i + 1; j < merged; j++)
            {
                if (candidateWeights[j] > candidateWeights[best])
                {
                    best = j;
                }
            }

            if (best != i)
            {
                (candidateIds[i], candidateIds[best]) = (candidateIds[best], candidateIds[i]);
                (candidateWeights[i], candidateWeights[best]) = (candidateWeights[best], candidateWeights[i]);
            }
        }

        int written = 0;
        for (int i = 0; i < merged && written < MaxActive && written < outIds.Length; i++)
        {
            if (candidateWeights[i] <= 0.0f)
            {
                break;
            }

            outIds[written] = candidateIds[i];
            outWeights[written] = candidateWeights[i];
            written++;
        }

        for (int i = written; i < outIds.Length; i++)
        {
            outIds[i] = 0;
            outWeights[i] = 0.0f;
        }

        return written;
    }

    public static uint Dominant(ReadOnlySpan<uint> ids, ReadOnlySpan<byte> weights)
    {
        Span<uint> seenIds = stackalloc uint[ValuesPerChunk];
        Span<int> seenWeights = stackalloc int[ValuesPerChunk];
        int seen = 0;
        for (int i = 0; i < ValuesPerChunk; i++)
        {
            uint id = ids[i];
            int weight = weights[i];
            if (id == 0 || weight == 0)
            {
                continue;
            }

            int found = -1;
            for (int j = 0; j < seen; j++)
            {
                if (seenIds[j] == id)
                {
                    found = j;
                    break;
                }
            }

            if (found >= 0)
            {
                seenWeights[found] += weight;
            }
            else
            {
                seenIds[seen] = id;
                seenWeights[seen] = weight;
                seen++;
            }
        }

        uint best = 0;
        int bestWeight = 0;
        for (int i = 0; i < seen; i++)
        {
            if (seenWeights[i] > bestWeight)
            {
                best = seenIds[i];
                bestWeight = seenWeights[i];
            }
        }

        return best;
    }

    public static int Normalise(Span<uint> ids, Span<float> weights)
    {
        float sum = 0.0f;
        for (int i = 0; i < ids.Length; i++)
        {
            if (ids[i] == 0)
            {
                weights[i] = 0.0f;
            }

            sum += weights[i];
        }

        if (sum <= WeightEpsilon)
        {
            ids[0] = DefaultSkyId;
            weights[0] = 1.0f;
            for (int i = 1; i < ids.Length; i++)
            {
                ids[i] = 0;
                weights[i] = 0.0f;
            }

            return 1;
        }

        int active = 0;
        for (int i = 0; i < ids.Length; i++)
        {
            if (ids[i] == 0)
            {
                continue;
            }

            weights[i] /= sum;
            if (active != i)
            {
                ids[active] = ids[i];
                weights[active] = weights[i];
                ids[i] = 0;
                weights[i] = 0.0f;
            }

            active++;
        }

        return active;
    }
}
