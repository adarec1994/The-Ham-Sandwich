using System;
using System.Collections.Generic;
using Godot;
using WildStar.Sky;

namespace WildStar.Area;

public static class AreaSceneBuilder
{
    public static AreaRoot Build(AreaFile area, string mapName, AreaTileCoord tile, string name, bool includeSky)
    {
        AreaTileCompute compute = AreaTileCompute.Compute(area, mapName, tile, name, true);

        var root = new AreaRoot
        {
            Name = name,
            MapName = mapName,
            TileX = tile.X,
            TileZ = tile.Z,
            LowDetail = tile.Low,
            ChunkCount = area.ChunkCount,
            Position = new Vector3(tile.OriginX, 0.0f, tile.OriginZ),
        };

        var chunks = new Node3D { Name = AreaRoot.ChunksNode };
        root.AddChild(chunks);
        chunks.Owner = root;

        TerrainSplat.TileMaps maps = tile.Low ? TerrainSplat.EmptyMaps : TerrainSplat.BuildTileMaps(compute);
        foreach (AreaChunkCompute chunk in compute.Chunks)
        {
            MeshInstance3D instance = BuildChunkInstance(chunk, mapName, tile);
            if (instance.Mesh is ArrayMesh chunkMesh)
            {
                chunkMesh.SurfaceSetMaterial(0, TerrainSplat.BuildMaterial(chunk.WorldLayerIds, maps));
            }

            chunks.AddChild(instance);
            instance.Owner = root;
        }

        var present = new List<int>();
        for (int i = 0; i < AreaLayerTable.Count; i++)
        {
            if ((compute.LayerUnion & (1u << i)) != 0)
            {
                present.Add(i);
            }
        }

        root.PresentLayers = present.ToArray();
        if (!tile.Low)
        {
            root.ChunkSkyIds = compute.ChunkSkyIds;
            root.ChunkSkyQuadrantIds = compute.ChunkSkyQuadrantIds;
            root.ChunkSkyQuadrantWeights = compute.ChunkSkyQuadrantWeights;
        }

        root.SkyId = (int)GameSky(compute.SkyVotes, name);
        root.SkyPath = ResolveSky((uint)root.SkyId, name);

        if (includeSky && root.SkyPath.Length > 0)
        {
            SkyRoot? sky = BuildSky(root.SkyPath);
            if (sky is not null)
            {
                sky.Name = AreaRoot.SkyNode;
                sky.Position = new Vector3(tile.TileSize * 0.5f, 0.0f, tile.TileSize * 0.5f);
                root.AddChild(sky);
                Own(sky, root);
            }
        }

        return root;
    }

    private static readonly HashSet<string> SkyFallbacksLogged = new(StringComparer.OrdinalIgnoreCase);

    public static string ResolveSky(uint id, string context)
    {
        if (id == 0)
        {
            return string.Empty;
        }

        string? path = AreaTables.ResolveSkyPath(id, out string label);
        if (!label.StartsWith("WorldSky", StringComparison.Ordinal) &&
            SkyFallbacksLogged.Add(context + ":" + id))
        {
            GD.Print($"[wildstar_mount] {context}: sky {label} -> {path ?? "<none>"}");
        }

        return path ?? string.Empty;
    }

    public static uint GameSky(Dictionary<uint, int> votes, string context)
    {
        foreach (uint id in RankedVotes(votes))
        {
            if (AreaTables.SkyPath(id) is not null)
            {
                return id;
            }

            if (SkyFallbacksLogged.Add(context + ":" + id))
            {
                GD.Print($"[wildstar_mount] {context}: WorldSky {id} has no record; skipped");
            }
        }

        return AreaTables.DefaultSkyId;
    }

    public static uint DominantSky(Dictionary<uint, int> votes)
    {
        foreach (uint id in RankedVotes(votes))
        {
            return id;
        }

        return 0u;
    }

    private static IEnumerable<uint> RankedVotes(Dictionary<uint, int> votes)
    {
        var ranked = new List<KeyValuePair<uint, int>>(votes);
        ranked.Sort((a, b) => b.Value != a.Value ? b.Value.CompareTo(a.Value) : a.Key.CompareTo(b.Key));
        foreach (KeyValuePair<uint, int> kv in ranked)
        {
            yield return kv.Key;
        }
    }

    public static SkyRoot? BuildSky(string skyPath)
    {
        byte[]? bytes = AreaTables.Read(skyPath);
        if (bytes is null)
        {
            GD.PushWarning("[wildstar_mount] sky " + skyPath + " not found");
            return null;
        }

        if (!SkyFile.TryParse(bytes, out SkyFile sky, out string error))
        {
            GD.PushWarning("[wildstar_mount] sky " + skyPath + ": " + error);
            return null;
        }

        int slash = skyPath.LastIndexOfAny(new[] { '/', '\\' });
        string leaf = slash >= 0 ? skyPath[(slash + 1)..] : skyPath;
        return SkySceneBuilder.Build(sky, bytes, leaf.EndsWith(".sky", StringComparison.OrdinalIgnoreCase) ? leaf[..^4] : leaf);
    }

    public static MeshInstance3D BuildChunkInstance(AreaChunkCompute chunk, string mapName, AreaTileCoord tile)
    {
        ArrayMesh mesh = MakeMeshFromArrays(chunk.Positions, chunk.Normals, chunk.Uvs, chunk.Indices,
                                            new Vector2(chunk.Chunk.Index, chunk.HasColourMap ? 1.0f : 0.0f));
        return new MeshInstance3D
        {
            Name = (tile.Low ? "low_" : "chunk_") + chunk.Chunk.X + "_" + chunk.Chunk.Y,
            Mesh = mesh,
            Position = new Vector3(chunk.OffsetX, 0.0f, chunk.OffsetZ),
        };
    }

    public static ArrayMesh MakeMeshFromArrays(float[] positions, float[] normals, float[] uvs, int[] indices,
                                               Vector2? uv2 = null)
    {
        int count = positions.Length / 3;
        var vertices = new Vector3[count];
        var vertexNormals = new Vector3[count];
        var vertexUvs = new Vector2[count];
        for (int v = 0; v < count; v++)
        {
            vertices[v] = new Vector3(positions[3 * v], positions[3 * v + 1], positions[3 * v + 2]);
            vertexNormals[v] = new Vector3(normals[3 * v], normals[3 * v + 1], normals[3 * v + 2]);
            vertexUvs[v] = new Vector2(uvs[2 * v], uvs[2 * v + 1]);
        }

        Vector2[]? uv2s = null;
        if (uv2 is Vector2 slot)
        {
            uv2s = new Vector2[count];
            Array.Fill(uv2s, slot);
        }

        return MakeMesh(vertices, vertexNormals, vertexUvs, indices, uv2s);
    }

    public static MeshInstance3D? BuildChunk(AreaChunk chunk, string mapName, AreaTileCoord tile)
    {
        if (!chunk.HasHeights)
        {
            return null;
        }

        AreaTerrain.BuildChunkGeometry(chunk, out float[] positions, out float[] normals, out bool[] holes);
        int[] indices = AreaTerrain.BuildChunkIndices(holes);
        if (indices.Length == 0)
        {
            return null;
        }

        var vertices = new Vector3[AreaTerrain.VertexCount];
        var vertexNormals = new Vector3[AreaTerrain.VertexCount];
        var uvs = new Vector2[AreaTerrain.VertexCount];
        for (int v = 0; v < AreaTerrain.VertexCount; v++)
        {
            vertices[v] = new Vector3(positions[3 * v], positions[3 * v + 1], positions[3 * v + 2]);
            vertexNormals[v] = new Vector3(normals[3 * v], normals[3 * v + 1], normals[3 * v + 2]);
            uvs[v] = new Vector2(positions[3 * v] / AreaTerrain.ChunkSize, positions[3 * v + 2] / AreaTerrain.ChunkSize);
        }

        ArrayMesh mesh = MakeMesh(vertices, vertexNormals, uvs, indices);
        return new MeshInstance3D
        {
            Name = "chunk_" + chunk.X + "_" + chunk.Y,
            Mesh = mesh,
            Position = new Vector3(chunk.X * AreaTerrain.ChunkSize, 0.0f, chunk.Y * AreaTerrain.ChunkSize),
        };
    }

    public static MeshInstance3D? BuildLowChunk(AreaChunk chunk, string mapName, AreaTileCoord tile)
    {
        if (!chunk.HasLowHeights)
        {
            return null;
        }

        float[] heights = AreaTerrain.LowHeights(chunk);
        const int side = AreaTerrain.LowVerticesPerSide;
        var vertices = new Vector3[side * side];
        var uvs = new Vector2[side * side];
        for (int y = 0; y < side; y++)
        {
            for (int x = 0; x < side; x++)
            {
                vertices[y * side + x] = new Vector3(x * AreaTerrain.LowCellSize, heights[y * side + x], y * AreaTerrain.LowCellSize);
                uvs[y * side + x] = new Vector2(x / (float)AreaTerrain.LowCellsPerChunk, y / (float)AreaTerrain.LowCellsPerChunk);
            }
        }

        var normals = new Vector3[side * side];
        for (int y = 0; y < side; y++)
        {
            for (int x = 0; x < side; x++)
            {
                float hl = heights[y * side + Math.Max(0, x - 1)];
                float hr = heights[y * side + Math.Min(side - 1, x + 1)];
                float hd = heights[Math.Max(0, y - 1) * side + x];
                float hu = heights[Math.Min(side - 1, y + 1) * side + x];
                normals[y * side + x] = new Vector3(hl - hr, 2.0f * AreaTerrain.LowCellSize, hd - hu).Normalized();
            }
        }

        var indices = new int[AreaTerrain.LowCellsPerChunk * AreaTerrain.LowCellsPerChunk * 6];
        int n = 0;
        for (int y = 0; y < AreaTerrain.LowCellsPerChunk; y++)
        {
            for (int x = 0; x < AreaTerrain.LowCellsPerChunk; x++)
            {
                int v00 = y * side + x;
                int v10 = v00 + 1;
                int v01 = v00 + side;
                int v11 = v01 + 1;
                indices[n++] = v00; indices[n++] = v10; indices[n++] = v01;
                indices[n++] = v10; indices[n++] = v11; indices[n++] = v01;
            }
        }

        ArrayMesh mesh = MakeMesh(vertices, normals, uvs, indices);
        return new MeshInstance3D
        {
            Name = "low_" + chunk.X + "_" + chunk.Y,
            Mesh = mesh,
            Position = new Vector3(chunk.X * AreaTerrain.LowChunkSize, 0.0f, chunk.Y * AreaTerrain.LowChunkSize),
        };
    }

    private static ArrayMesh MakeMesh(Vector3[] vertices, Vector3[] normals, Vector2[] uvs, int[] indices,
                                      Vector2[]? uv2s = null)
    {
        var arrays = new Godot.Collections.Array();
        arrays.Resize((int)Mesh.ArrayType.Max);
        arrays[(int)Mesh.ArrayType.Vertex] = vertices;
        arrays[(int)Mesh.ArrayType.Normal] = normals;
        arrays[(int)Mesh.ArrayType.TexUV] = uvs;
        if (uv2s is not null)
        {
            arrays[(int)Mesh.ArrayType.TexUV2] = uv2s;
        }

        arrays[(int)Mesh.ArrayType.Index] = indices;
        var mesh = new ArrayMesh();
        mesh.AddSurfaceFromArrays(Mesh.PrimitiveType.Triangles, arrays);
        return mesh;
    }

    public static void Own(Node node, Node owner)
    {
        var stack = new Stack<Node>();
        stack.Push(node);
        while (stack.Count > 0)
        {
            Node current = stack.Pop();
            current.Owner = owner;
            foreach (Node child in current.GetChildren())
            {
                stack.Push(child);
            }
        }
    }
}
