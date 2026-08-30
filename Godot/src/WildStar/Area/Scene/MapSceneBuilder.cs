using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.Linq;
using System.Threading.Tasks;
using Godot;
using WildStar.Archive;
using WildStar.Model;
using WildStar.Sky;

namespace WildStar.Area;

public static class MapSceneBuilder
{
    public const int FallbackRadius = 3;

    private const int BigMapTiles = 300;

    public readonly struct TileEntry
    {
        public TileEntry(WsFile file, AreaTileCoord coord)
        {
            File = file;
            Coord = coord;
        }

        public WsFile File { get; }

        public AreaTileCoord Coord { get; }
    }

    public static bool TryListTiles(string mapName, out List<TileEntry> tiles, out string error)
    {
        tiles = new List<TileEntry>();
        WsFileSystem? fs = AreaTables.FileSystem;
        if (fs is null)
        {
            error = "no archives mounted";
            return false;
        }

        WsDirectory? folder = null;
        foreach (WsArchive archive in fs.Archives)
        {
            if (fs.TryGetDirectory(archive.Name + "://Map/" + mapName, out WsDirectory found))
            {
                folder = found;
                break;
            }
        }

        if (folder is null)
        {
            error = "Map/" + mapName + " not found";
            return false;
        }

        foreach (WsFile file in folder.Files)
        {
            if (!AreaTileCoord.TryParse(file.Name, out string name, out AreaTileCoord coord))
            {
                continue;
            }

            if (!string.Equals(name, mapName, StringComparison.OrdinalIgnoreCase))
            {
                continue;
            }

            tiles.Add(new TileEntry(file, coord));
        }

        error = string.Empty;
        return true;
    }

    public const int BatchSize = 24;

    public static MapRoot BuildStreaming(string mapName, float streamRadius, Action<string>? progress = null)
    {
        var root = new MapRoot { Name = mapName, MapName = mapName, DetailRadius = -1 };
        if (!TryListTiles(mapName, out List<TileEntry> tiles, out string error))
        {
            GD.PushWarning("[wildstar_mount] " + error);
            return root;
        }

        int detail = 0, low = 0;
        long sx = 0, sz = 0;
        foreach (TileEntry t in tiles)
        {
            if (t.Coord.Low)
            {
                low++;
                continue;
            }

            detail++;
            sx += t.Coord.X;
            sz += t.Coord.Z;
        }

        root.FocusX = detail > 0 ? (int)(sx / detail) : AreaTerrain.TileOriginOffset;
        root.FocusZ = detail > 0 ? (int)(sz / detail) : AreaTerrain.TileOriginOffset;
        root.StreamRadius = streamRadius;
        root.SkyId = 0;
        AreaTables.TryWorldLayer(1, out _);

        progress?.Invoke($"{mapName}: {detail} detail + {low} low tiles; streaming chunks within " +
                         $"{streamRadius:F0} units of the camera, `_Low` beyond");
        return root;
    }

    public static MapRoot Build(string mapName, int focusX, int focusZ, int detailRadius, bool autoFocus,
                                Action<string>? progress = null)
    {
        var total = Stopwatch.StartNew();
        TerrainSplat.ResetCounters();
        MeshTicks = 0;
        SurfaceTotal = 0;
        var root = new MapRoot { Name = mapName, MapName = mapName, DetailRadius = detailRadius };
        if (!TryListTiles(mapName, out List<TileEntry> tiles, out string error))
        {
            GD.PushWarning("[wildstar_mount] " + error);
            return root;
        }

        var detail = new List<TileEntry>();
        foreach (TileEntry t in tiles)
        {
            if (!t.Coord.Low)
            {
                detail.Add(t);
            }
        }

        int detailCount = detail.Count;
        bool windowed = detailRadius >= 0;
        var wanted = new List<TileEntry>();

        if (!windowed)
        {
            wanted.AddRange(detail);
            progress?.Invoke($"{mapName}: loading all {detailCount} detail tiles" +
                             (detailCount > BigMapTiles
                                 ? " (~9 MB of mesh each — a big map can run into several GB)"
                                 : string.Empty));
        }
        else
        {
            progress?.Invoke($"{mapName}: radius {detailRadius} around ({focusX},{focusZ}) of " +
                             $"{detailCount} detail tiles");

            foreach (TileEntry t in detail)
            {
                if (Math.Abs(t.Coord.X - focusX) <= detailRadius &&
                    Math.Abs(t.Coord.Z - focusZ) <= detailRadius)
                {
                    wanted.Add(t);
                }
            }
        }

        root.DetailRadius = windowed ? detailRadius : -1;

        var compute = Stopwatch.StartNew();
        var assemble = Stopwatch.StartNew();
        compute.Stop();
        assemble.Stop();

        var skyVotes = new Dictionary<uint, int>();
        int failed = 0, detailBuilt = 0, chunkTotal = 0, bareChunks = 0, built = 0;

        long paintedX = 0, paintedZ = 0, paintedTiles = 0;

        for (int batchStart = 0; batchStart < wanted.Count; batchStart += BatchSize)
        {
            int batchCount = Math.Min(BatchSize, wanted.Count - batchStart);
            var computed = new AreaTileCompute?[batchCount];

            compute.Start();
            Parallel.For(0, batchCount,
                new ParallelOptions { MaxDegreeOfParallelism = System.Environment.ProcessorCount },
                i =>
            {
                TileEntry t = wanted[batchStart + i];
                try
                {
                    byte[] bytes = t.File.ReadAllBytes();
                    if (!AreaFile.TryParse(bytes, out AreaFile area, out string parseError))
                    {
                        GD.PushWarning("[wildstar_mount] " + t.File.QualifiedPath + ": " + parseError);
                        System.Threading.Interlocked.Increment(ref failed);
                        return;
                    }

                    computed[i] = AreaTileCompute.Compute(area, mapName, t.Coord, t.File.Name[..^5], false);
                }
                catch (Exception exception)
                {
                    GD.PushWarning("[wildstar_mount] " + t.File.QualifiedPath + ": " + exception.Message);
                    System.Threading.Interlocked.Increment(ref failed);
                }
            });
            compute.Stop();


            assemble.Start();
            for (int i = 0; i < batchCount; i++)
            {
                AreaTileCompute? tileCompute = computed[i];
                if (tileCompute is null)
                {
                    continue;
                }

                PendingTile pending = AssembleTile(tileCompute);
                AreaRoot tile = pending.Root;
                foreach (ChunkNode node in pending.Chunks)
                {
                    tile.AddChild(node.Instance);
                    node.Instance.Owner = tile;
                }

                root.AddChild(tile);
                AreaSceneBuilder.Own(tile, root);
                chunkTotal += tileCompute.Chunks.Count;
                int painted = 0;
                foreach (AreaChunkCompute chunk in tileCompute.Chunks)
                {
                    uint[] ids = chunk.WorldLayerIds;
                    if (ids[0] == 0 && ids[1] == 0 && ids[2] == 0 && ids[3] == 0)
                    {
                        bareChunks++;
                    }
                    else
                    {
                        painted++;
                    }
                }

                if (painted > 0)
                {
                    paintedX += tileCompute.Tile.X;
                    paintedZ += tileCompute.Tile.Z;
                    paintedTiles++;
                }

                if (tile.GetNodeOrNull<Node3D>(AreaRoot.ChunksNode)?.GetChildOrNull<MeshInstance3D>(0)?.Mesh is ArrayMesh am)
                {
                    SurfaceTotal += am.GetSurfaceCount();
                }

                detailBuilt++;
                foreach (KeyValuePair<uint, int> vote in tileCompute.SkyVotes)
                {
                    skyVotes[vote.Key] = skyVotes.TryGetValue(vote.Key, out int n) ? n + vote.Value : vote.Value;
                }

                computed[i] = null;
                built++;
            }

            assemble.Stop();
            progress?.Invoke($"{built}/{wanted.Count} tiles…");
        }

        if (autoFocus && paintedTiles > 0)
        {
            focusX = (int)(paintedX / paintedTiles);
            focusZ = (int)(paintedZ / paintedTiles);
        }

        root.FocusX = focusX;
        root.FocusZ = focusZ;

        root.DetailRange = 0.0f;
        root.DetailTiles = detailBuilt;
        root.LowTiles = 0;
        root.SkyId = (int)AreaSceneBuilder.GameSky(skyVotes, mapName);
        string resolved = AreaSceneBuilder.ResolveSky((uint)root.SkyId, mapName);
        string? skyPath = resolved.Length > 0 ? resolved : null;
        if (skyPath is not null)
        {
            SkyRoot? sky = AreaSceneBuilder.BuildSky(skyPath);
            if (sky is not null)
            {
                sky.Name = AreaRoot.SkyNode;
                var centre = new AreaTileCoord(focusX, focusZ, false);
                sky.Position = new Vector3(centre.OriginX + AreaTerrain.TileSize * 0.5f, 0.0f,
                                           centre.OriginZ + AreaTerrain.TileSize * 0.5f);
                root.AddChild(sky);
                AreaSceneBuilder.Own(sky, root);
            }
        }

        var skyLoad = Stopwatch.StartNew();
        int preloaded = root.PreloadSkies(m => progress?.Invoke("  " + m));
        skyLoad.Stop();

        total.Stop();
        progress?.Invoke($"  [timing] skies: {preloaded} built in {skyLoad.ElapsedMilliseconds} ms; meshes " +
                         (MeshTicks * 1000 / System.Diagnostics.Stopwatch.Frequency) + " ms");
        progress?.Invoke("  [timing] " + TerrainSplat.TimingReport());
        progress?.Invoke($"{mapName}: {detailBuilt} tiles" +
                         (detailBuilt < detailCount ? $" of {detailCount}" : string.Empty) +
                         $", {chunkTotal} chunks in {SurfaceTotal} surfaces, sky {root.SkyId}" +
                         (failed > 0 ? $", {failed} failed" : string.Empty) +
                         $" — compute {compute.ElapsedMilliseconds} ms (parallel), assemble {assemble.ElapsedMilliseconds} ms, " +
                         $"total {total.ElapsedMilliseconds} ms");
        progress?.Invoke($"{mapName}: splat coverage {chunkTotal - bareChunks}/{chunkTotal} chunks " +
                         $"({(chunkTotal > 0 ? 100.0 * (chunkTotal - bareChunks) / chunkTotal : 0.0):F1}%)" +
                         (bareChunks > 0
                             ? $"; {bareChunks} chunks carry no WorldLayer ids in the .area and draw on the " +
                               "viewer's white fallback"
                             : string.Empty));
        return root;
    }

    internal static PendingTile? BuildTile(string mapName, TileEntry entry)
    {
        try
        {
            if (!AreaFile.TryParse(entry.File.ReadAllBytes(), out AreaFile area, out _))
            {
                return null;
            }

            AreaTileCompute compute = AreaTileCompute.Compute(
                area, mapName, entry.Coord, entry.File.Name[..^5], !entry.Coord.Low);
            if (compute.Chunks.Count == 0)
            {
                return null;
            }

            if (entry.Coord.Low)
            {
                return AssembleLowTile(compute);
            }

            var ids = new HashSet<uint>();
            foreach (AreaChunkCompute chunk in compute.Chunks)
            {
                foreach (uint id in chunk.WorldLayerIds)
                {
                    ids.Add(id);
                }
            }

            TerrainSplat.PrewarmLayers(ids);
            return AssembleTile(compute);
        }
        catch (Exception exception)
        {
            GD.PushWarning("[wildstar_mount] " + entry.File.QualifiedPath + ": " + exception.Message);
            return null;
        }
    }

    internal sealed class PendingTile
    {
        public PendingTile(AreaRoot root, List<ChunkNode> chunks)
        {
            Root = root;
            Chunks = chunks;
        }

        public AreaRoot Root { get; }

        public List<ChunkNode> Chunks { get; }
    }

    internal readonly struct ChunkNode
    {
        public ChunkNode(MeshInstance3D instance, Vector3 centre)
        {
            Instance = instance;
            Centre = centre;
        }

        public MeshInstance3D Instance { get; }

        public Vector3 Centre { get; }
    }

    private static AreaRoot NewTileRoot(AreaTileCompute compute)
    {
        AreaTileCoord tile = compute.Tile;
        var root = new AreaRoot
        {
            Name = compute.Name.Replace('.', '_'),
            MapName = compute.MapName,
            TileX = tile.X,
            TileZ = tile.Z,
            LowDetail = tile.Low,
            ChunkCount = compute.Area.ChunkCount,
            Position = new Vector3(tile.OriginX, 0.0f, tile.OriginZ),
        };

        var present = new List<int>();
        for (int i = 0; i < AreaLayerTable.Count; i++)
        {
            if ((compute.LayerUnion & (1u << i)) != 0)
            {
                present.Add(i);
            }
        }

        root.PresentLayers = present.ToArray();
        root.SkyId = 0;
        if (!tile.Low)
        {
            root.ChunkSkyIds = compute.ChunkSkyIds;
            root.ChunkSkyQuadrantIds = compute.ChunkSkyQuadrantIds;
            root.ChunkSkyQuadrantWeights = compute.ChunkSkyQuadrantWeights;
        }

        return root;
    }

    internal static PendingTile AssembleTile(AreaTileCompute compute)
    {
        AreaRoot root = NewTileRoot(compute);
        var nodes = new List<ChunkNode>(compute.Chunks.Count);
        if (compute.Chunks.Count == 0)
        {
            return new PendingTile(root, nodes);
        }

        TerrainSplat.TileMaps maps = TerrainSplat.BuildTileMaps(compute);
        var materials = new Dictionary<(uint, uint, uint, uint), ShaderMaterial>();

        foreach (AreaChunkCompute chunk in compute.Chunks)
        {
            (uint, uint, uint, uint) key = TerrainSplat.Signature(chunk);
            if (!materials.TryGetValue(key, out ShaderMaterial? material))
            {
                material = TerrainSplat.BuildMaterial(
                    new[] { key.Item1, key.Item2, key.Item3, key.Item4 }, maps);
                materials[key] = material;
            }

            var mesh = new ArrayMesh();
            AddChunkSurface(mesh, chunk, Vector2.One, Vector2.Zero);
            mesh.SurfaceSetMaterial(0, material);
            nodes.Add(new ChunkNode(
                new MeshInstance3D { Name = "c" + chunk.Chunk.Index, Mesh = mesh },
                new Vector3(compute.Tile.OriginX + chunk.OffsetX + (AreaTerrain.ChunkSize * 0.5f), 0.0f,
                            compute.Tile.OriginZ + chunk.OffsetZ + (AreaTerrain.ChunkSize * 0.5f))));
        }

        return new PendingTile(root, nodes);
    }

    internal static PendingTile AssembleLowTile(AreaTileCompute compute)
    {
        AreaRoot root = NewTileRoot(compute);
        var nodes = new List<ChunkNode>(compute.Chunks.Count);
        if (compute.Chunks.Count == 0)
        {
            return new PendingTile(root, nodes);
        }

        var materials = new Dictionary<(int X, int Z), ShaderMaterial>();
        foreach (AreaChunkCompute chunk in compute.Chunks)
        {
            var source = (X: (compute.Tile.X * 8) + (chunk.Chunk.X / 2),
                          Z: (compute.Tile.Z * 8) + (chunk.Chunk.Y / 2));
            if (!materials.TryGetValue(source, out ShaderMaterial? material))
            {
                material = TerrainSplat.BuildLowMaterial(
                    TerrainSplat.Composite(compute.MapName, new AreaTileCoord(source.X, source.Z, false)));
                materials[source] = material;
            }

            var offset = new Vector2((chunk.Chunk.X & 1) * 0.5f, (chunk.Chunk.Y & 1) * 0.5f);
            var mesh = new ArrayMesh();
            AddChunkSurface(mesh, chunk, new Vector2(0.5f, 0.5f), offset);
            mesh.SurfaceSetMaterial(0, material);
            nodes.Add(new ChunkNode(
                new MeshInstance3D { Name = "l" + chunk.Chunk.Index, Mesh = mesh },
                new Vector3(compute.Tile.OriginX + chunk.OffsetX + (AreaTerrain.LowChunkSize * 0.5f), 0.0f,
                            compute.Tile.OriginZ + chunk.OffsetZ + (AreaTerrain.LowChunkSize * 0.5f))));
        }

        return new PendingTile(root, nodes);
    }

    private static void AddChunkSurface(ArrayMesh mesh, AreaChunkCompute chunk,
                                        Vector2 uvScale, Vector2 uvOffset)
    {
        int count = chunk.Positions.Length / 3;
        var vertices = new Vector3[count];
        var normals = new Vector3[count];
        var uvs = new Vector2[count];
        var uv2s = new Vector2[count];
        var slot = new Vector2(chunk.Chunk.Index, chunk.HasColourMap ? 1.0f : 0.0f);

        for (int v = 0; v < count; v++)
        {
            vertices[v] = new Vector3(
                chunk.Positions[3 * v] + chunk.OffsetX,
                chunk.Positions[3 * v + 1],
                chunk.Positions[3 * v + 2] + chunk.OffsetZ);
            normals[v] = new Vector3(chunk.Normals[3 * v], chunk.Normals[3 * v + 1], chunk.Normals[3 * v + 2]);
            uvs[v] = new Vector2((chunk.Uvs[2 * v] * uvScale.X) + uvOffset.X,
                                 (chunk.Uvs[2 * v + 1] * uvScale.Y) + uvOffset.Y);
            uv2s[v] = slot;
        }

        var arrays = new Godot.Collections.Array();
        arrays.Resize((int)Mesh.ArrayType.Max);
        arrays[(int)Mesh.ArrayType.Vertex] = vertices;
        arrays[(int)Mesh.ArrayType.Normal] = normals;
        arrays[(int)Mesh.ArrayType.TexUV] = uvs;
        arrays[(int)Mesh.ArrayType.TexUV2] = uv2s;
        arrays[(int)Mesh.ArrayType.Index] = chunk.Indices;
        mesh.AddSurfaceFromArrays(Mesh.PrimitiveType.Triangles, arrays);
    }

    private static void AddMergedSurface(ArrayMesh mesh, List<AreaChunkCompute> chunks)
    {
        var sw = System.Diagnostics.Stopwatch.StartNew();
        int vertexCount = 0, indexCount = 0;
        foreach (AreaChunkCompute chunk in chunks)
        {
            vertexCount += chunk.Positions.Length / 3;
            indexCount += chunk.Indices.Length;
        }

        var vertices = new Vector3[vertexCount];
        var normals = new Vector3[vertexCount];
        var uvs = new Vector2[vertexCount];
        var uv2s = new Vector2[vertexCount];
        var indices = new int[indexCount];

        int vBase = 0, iBase = 0;
        foreach (AreaChunkCompute chunk in chunks)
        {
            int count = chunk.Positions.Length / 3;
            var slot = new Vector2(chunk.Chunk.Index, chunk.HasColourMap ? 1.0f : 0.0f);
            for (int v = 0; v < count; v++)
            {
                vertices[vBase + v] = new Vector3(
                    chunk.Positions[3 * v] + chunk.OffsetX,
                    chunk.Positions[3 * v + 1],
                    chunk.Positions[3 * v + 2] + chunk.OffsetZ);
                normals[vBase + v] = new Vector3(
                    chunk.Normals[3 * v], chunk.Normals[3 * v + 1], chunk.Normals[3 * v + 2]);
                uvs[vBase + v] = new Vector2(chunk.Uvs[2 * v], chunk.Uvs[2 * v + 1]);
                uv2s[vBase + v] = slot;
            }

            for (int i = 0; i < chunk.Indices.Length; i++)
            {
                indices[iBase + i] = chunk.Indices[i] + vBase;
            }

            vBase += count;
            iBase += chunk.Indices.Length;
        }

        BuildTicks += sw.ElapsedTicks;

        var pack = System.Diagnostics.Stopwatch.StartNew();
        var arrays = new Godot.Collections.Array();
        arrays.Resize((int)Mesh.ArrayType.Max);
        arrays[(int)Mesh.ArrayType.Vertex] = vertices;
        arrays[(int)Mesh.ArrayType.Normal] = normals;
        arrays[(int)Mesh.ArrayType.TexUV] = uvs;
        arrays[(int)Mesh.ArrayType.TexUV2] = uv2s;
        arrays[(int)Mesh.ArrayType.Index] = indices;
        PackTicks += pack.ElapsedTicks;

        pack.Restart();
        mesh.AddSurfaceFromArrays(Mesh.PrimitiveType.Triangles, arrays);
        UploadTicks += pack.ElapsedTicks;
        MeshTicks += sw.ElapsedTicks;
    }

    public static long MeshTicks;

    public static long BuildTicks, PackTicks, UploadTicks;

    public static int SurfaceTotal;
}
