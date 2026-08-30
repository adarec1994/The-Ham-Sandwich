using System;
using Godot;

namespace WildStar.Area;

[Tool]
[GlobalClass]
public partial class AreaRoot : Node3D
{
    public const string ChunksNode = "Chunks";
    public const string SkyNode = "Sky";

    [Export] public string MapName { get; set; } = string.Empty;

    [Export] public int TileX { get; set; }

    [Export] public int TileZ { get; set; }

    [Export] public bool LowDetail { get; set; }

    [Export] public int ChunkCount { get; set; }

    [Export] public int SkyId { get; set; }

    [Export] public string SkyPath { get; set; } = string.Empty;

    [Export] public int[] PresentLayers { get; set; } = System.Array.Empty<int>();

    [Export] public int[] ChunkSkyIds { get; set; } = System.Array.Empty<int>();

    [Export] public int[] ChunkSkyQuadrantIds { get; set; } = System.Array.Empty<int>();

    [Export] public byte[] ChunkSkyQuadrantWeights { get; set; } = System.Array.Empty<byte>();

    public int SampleSky(float tileX, float tileZ, Span<uint> outIds, Span<float> outWeights)
    {
        if (ChunkSkyQuadrantIds.Length != 256 * AreaSky.ValuesPerChunk ||
            ChunkSkyQuadrantWeights.Length != 256 * AreaSky.ValuesPerChunk)
        {
            outIds.Clear();
            outWeights.Clear();
            return 0;
        }

        int chunkX = Mathf.Clamp((int)(tileX / AreaTerrain.ChunkSize), 0, 15);
        int chunkZ = Mathf.Clamp((int)(tileZ / AreaTerrain.ChunkSize), 0, 15);
        float fx = Mathf.Clamp(tileX / AreaTerrain.ChunkSize - chunkX, 0.0f, 1.0f);
        float fz = Mathf.Clamp(tileZ / AreaTerrain.ChunkSize - chunkZ, 0.0f, 1.0f);
        int baseIndex = (chunkZ * 16 + chunkX) * AreaSky.ValuesPerChunk;
        Span<uint> ids = stackalloc uint[AreaSky.ValuesPerChunk];
        for (int i = 0; i < AreaSky.ValuesPerChunk; i++)
        {
            ids[i] = (uint)ChunkSkyQuadrantIds[baseIndex + i];
        }

        return AreaSky.Sample(ids, ChunkSkyQuadrantWeights.AsSpan(baseIndex, AreaSky.ValuesPerChunk), fx, fz,
                              outIds, outWeights);
    }
}
