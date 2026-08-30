using System;
using System.Collections.Generic;
using System.Linq;
using Godot;
using WildStar.Texture;

namespace WildStar.Area;

public static class TerrainSplat
{
    public const string ShaderCode = @"shader_type spatial;
render_mode unshaded, cull_back, depth_draw_opaque;

uniform sampler2DArray blend_maps : filter_linear, repeat_disable;
uniform sampler2DArray colour_maps : filter_linear, repeat_disable;
uniform float blend_uv_scale = 1.0;
uniform float colour_uv_scale = 1.0;
uniform float blend_alpha_from_rgb = 0.0;
uniform sampler2D layer0 : filter_linear_mipmap_anisotropic, repeat_enable;
uniform sampler2D layer1 : filter_linear_mipmap_anisotropic, repeat_enable;
uniform sampler2D layer2 : filter_linear_mipmap_anisotropic, repeat_enable;
uniform sampler2D layer3 : filter_linear_mipmap_anisotropic, repeat_enable;
uniform sampler2D layer0_normal : filter_linear_mipmap_anisotropic, repeat_enable;
uniform sampler2D layer1_normal : filter_linear_mipmap_anisotropic, repeat_enable;
uniform sampler2D layer2_normal : filter_linear_mipmap_anisotropic, repeat_enable;
uniform sampler2D layer3_normal : filter_linear_mipmap_anisotropic, repeat_enable;
uniform vec4 tex_scale = vec4(8.0);
uniform vec4 base_color = vec4(1.0);
uniform vec4 highlight_color = vec4(0.0);

varying vec3 world_normal;
varying float chunk_index;
varying float has_color_map;

vec3 to_linear(vec3 c) {
    vec3 lo = c / 12.92;
    vec3 hi = pow((c + vec3(0.055)) / 1.055, vec3(2.4));
    return mix(lo, hi, step(vec3(0.04045), c));
}

vec3 sample_normal(sampler2D normal_tex, vec2 uv) {
    vec3 n = texture(normal_tex, uv).rgb;
    return normalize(n * 2.0 - 1.0);
}

void vertex() {
    world_normal = normalize(MODEL_NORMAL_MATRIX * NORMAL);
    chunk_index = UV2.x;
    has_color_map = UV2.y;
}

void fragment() {
    vec4 blend = texture(blend_maps, vec3(UV * blend_uv_scale, chunk_index));
    if (blend_alpha_from_rgb > 0.5) {
        blend.a = max(0.0, 1.0 - (blend.r + blend.g + blend.b));
    }

    float blendSum = blend.r + blend.g + blend.b + blend.a;
    if (blendSum > 0.001) {
        blend /= blendSum;
    } else {
        blend = vec4(1.0, 0.0, 0.0, 0.0);
    }

    vec2 uv0 = UV * tex_scale.x;
    vec2 uv1 = UV * tex_scale.y;
    vec2 uv2 = UV * tex_scale.z;
    vec2 uv3 = UV * tex_scale.w;

    vec4 col0 = texture(layer0, uv0);
    vec4 col1 = texture(layer1, uv1);
    vec4 col2 = texture(layer2, uv2);
    vec4 col3 = texture(layer3, uv3);

    vec4 diffuse = col0 * blend.r + col1 * blend.g + col2 * blend.b + col3 * blend.a;

    vec3 n0 = sample_normal(layer0_normal, uv0);
    vec3 n1 = sample_normal(layer1_normal, uv1);
    vec3 n2 = sample_normal(layer2_normal, uv2);
    vec3 n3 = sample_normal(layer3_normal, uv3);

    vec3 blendedNormal = normalize(n0 * blend.r + n1 * blend.g + n2 * blend.b + n3 * blend.a);

    vec3 N = normalize(world_normal);

    vec3 worldNormal = normalize(vec3(
        N.x + blendedNormal.x * 0.15,
        N.y,
        N.z + blendedNormal.y * 0.15));

    if (has_color_map > 0.5) {
        vec4 tint = texture(colour_maps, vec3(UV * colour_uv_scale, chunk_index));
        diffuse.rgb *= tint.rgb * 2.0;
    }

    vec3 lightDir = normalize(vec3(0.5, 1.0, 0.3));
    float NdotL = max(dot(worldNormal, lightDir), 0.0);

    vec3 ambient = vec3(0.3, 0.3, 0.3);
    vec3 lighting = ambient + vec3(0.7, 0.7, 0.7) * NdotL;

    vec3 finalColor = diffuse.rgb * lighting * base_color.rgb;

    if (highlight_color.a > 0.0) {
        finalColor = mix(finalColor, highlight_color.rgb, highlight_color.a * 0.3);
    }

    ALBEDO = to_linear(clamp(finalColor, vec3(0.0), vec3(1.0)));
}
";

    public const string LowShaderCode = @"shader_type spatial;
render_mode unshaded, cull_back, depth_draw_opaque;

uniform sampler2D composite : filter_linear_mipmap, repeat_disable;
uniform vec4 base_color = vec4(1.0);

varying vec3 world_normal;

vec3 to_linear(vec3 c) {
    vec3 lo = c / 12.92;
    vec3 hi = pow((c + vec3(0.055)) / 1.055, vec3(2.4));
    return mix(lo, hi, step(vec3(0.04045), c));
}

void vertex() {
    world_normal = normalize(MODEL_NORMAL_MATRIX * NORMAL);
}

void fragment() {
    vec3 albedo = texture(composite, UV).rgb;
    vec3 lightDir = normalize(vec3(0.5, 1.0, 0.3));
    float NdotL = max(dot(normalize(world_normal), lightDir), 0.0);
    vec3 lighting = vec3(0.3) + vec3(0.7) * NdotL;
    ALBEDO = to_linear(clamp(albedo * lighting * base_color.rgb, vec3(0.0), vec3(1.0)));
}
";

    public const int MapTexels = AreaChunkCompute.MapSize;
    public const int DxtTexels = 68;
    public const int BlendDxtBytes = 2312;
    public const int ColourDxtBytes = 4624;

    public sealed class CachedTexture
    {
        public Texture2D Diffuse = null!;
        public Texture2D Normal = null!;
        public int Width;
        public int Height;
    }

    public sealed class TileMaps
    {
        public Texture2DArray Blend = null!;
        public Texture2DArray Colour = null!;
        public bool BlendDxt;
        public bool ColourDxt;
    }

    private static Shader? _shader;

    private static Shader? _lowShader;

    public static int MaxLayerTexels = 512;

    private static readonly System.Collections.Concurrent.ConcurrentDictionary<uint, CachedTexture>
        TextureCache = new();

    private static readonly System.Collections.Concurrent.ConcurrentDictionary<string, Texture2D?>
        PathTextureCache = new(StringComparer.OrdinalIgnoreCase);

    private static readonly System.Collections.Concurrent.ConcurrentDictionary<string, Lazy<CpuLayer?>>
        LayerBlobs = new(StringComparer.OrdinalIgnoreCase);

    private sealed class CpuLayer
    {
        public CpuLayer(int width, int height, bool mipmaps, Image.Format format, byte[] data)
        {
            Width = width;
            Height = height;
            Mipmaps = mipmaps;
            Format = format;
            Data = data;
        }

        public int Width { get; }

        public int Height { get; }

        public bool Mipmaps { get; }

        public Image.Format Format { get; }

        public byte[] Data { get; }
    }
    private static ImageTexture? _fallbackWhite;
    private static ImageTexture? _fallbackNormal;
    private static byte[]? _defaultBlendDxt;
    private static byte[]? _defaultColourDxt;
    private static TileMaps? _emptyMaps;

    public static int LayerTextureCount => PathTextureCache.Count;

    public static long LayerTextureBytes;

    public static long MapTicks;

    public static long LayerTicks;

    public static int TileMapsBuilt;

    public static int MaterialsBuilt;

    public static void ResetCounters()
    {
        MapTicks = 0;
        LayerTicks = 0;
        TileMapsBuilt = 0;
        MaterialsBuilt = 0;
    }

    public static string TimingReport() =>
        $"splat: {TileMapsBuilt} tile map arrays in {MapTicks * 1000 / System.Diagnostics.Stopwatch.Frequency} ms, " +
        $"{TextureCache.Count} WorldLayer textures in {LayerTicks * 1000 / System.Diagnostics.Stopwatch.Frequency} ms, " +
        $"{MaterialsBuilt} materials";

    public static Shader Shader
    {
        get { lock (FallbackGate) { return _shader ??= new Shader { Code = ShaderCode }; } }
    }

    public static Shader LowShader
    {
        get { lock (FallbackGate) { return _lowShader ??= new Shader { Code = LowShaderCode }; } }
    }

    public static void ClearCache()
    {
        TextureCache.Clear();
        PathTextureCache.Clear();
        Composites.Clear();
        LayerBlobs.Clear();
        LayerTextureBytes = 0;
        _fallbackWhite = null;
        _fallbackNormal = null;
        _emptyMaps = null;
    }

    public static TileMaps EmptyMaps => _emptyMaps ??= BuildTileMaps(null);

    public static CachedTexture GetLayerTexture(uint layerId)
    {
        if (TextureCache.TryGetValue(layerId, out CachedTexture? cached))
        {
            return cached;
        }

        var sw = System.Diagnostics.Stopwatch.StartNew();
        var tex = new CachedTexture();
        if (!AreaTables.TryWorldLayer(layerId, out WorldLayerEntry entry))
        {
            tex.Diffuse = FallbackWhite;
            tex.Normal = FallbackNormal;
            tex.Width = 1;
            tex.Height = 1;
            TextureCache[layerId] = tex;
            LayerTicks += sw.ElapsedTicks;
            return tex;
        }

        if (entry.DiffusePath.Length > 0)
        {
            Texture2D? diffuse = LoadTextureFromPath(entry.DiffusePath);
            if (diffuse is not null)
            {
                tex.Diffuse = diffuse;
                tex.Width = diffuse.GetWidth();
                tex.Height = diffuse.GetHeight();
            }
        }

        if (entry.NormalPath.Length > 0)
        {
            Texture2D? normal = LoadTextureFromPath(entry.NormalPath);
            if (normal is not null)
            {
                tex.Normal = normal;
            }
        }

        if (tex.Diffuse is null)
        {
            tex.Diffuse = FallbackWhite;
            tex.Width = 1;
            tex.Height = 1;
        }

        tex.Normal ??= FallbackNormal;
        TextureCache[layerId] = tex;
        LayerTicks += sw.ElapsedTicks;
        return tex;
    }

    public static void PrewarmLayers(IEnumerable<uint> layerIds)
    {
        foreach (uint id in layerIds)
        {
            if (id == 0 || !AreaTables.TryWorldLayer(id, out WorldLayerEntry entry))
            {
                continue;
            }

            if (entry.DiffusePath.Length > 0 && !PathTextureCache.ContainsKey(entry.DiffusePath))
            {
                Prepare(entry.DiffusePath);
            }

            if (entry.NormalPath.Length > 0 && !PathTextureCache.ContainsKey(entry.NormalPath))
            {
                Prepare(entry.NormalPath);
            }
        }
    }

    private static CpuLayer? Prepare(string path) =>
        LayerBlobs.GetOrAdd(path,
            key => new Lazy<CpuLayer?>(() => Decode(key),
                                       System.Threading.LazyThreadSafetyMode.ExecutionAndPublication)).Value;

    private static Texture2D? LoadTextureFromPath(string path)
    {
        if (PathTextureCache.TryGetValue(path, out Texture2D? cached))
        {
            return cached;
        }

        CpuLayer? cpu = Prepare(path);
        LayerBlobs.TryRemove(path, out _);

        Texture2D? texture = cpu is null
            ? null
            : ImageTexture.CreateFromImage(
                Image.CreateFromData(cpu.Width, cpu.Height, cpu.Mipmaps, cpu.Format, cpu.Data));

        PathTextureCache[path] = texture;
        if (cpu is not null)
        {
            System.Threading.Interlocked.Add(ref LayerTextureBytes, cpu.Data.Length);
        }

        return texture;
    }

    private static CpuLayer? Decode(string path)
    {
        byte[]? bytes = AreaTables.Read(path);
        if (bytes is null)
        {
            return null;
        }

        bool normalMap = bytes.Length > 0x28 &&
                         System.Buffers.Binary.BinaryPrimitives.ReadUInt32LittleEndian(
                             bytes.AsSpan(0x24)) == TexFile.NormalPixelMode;
        if (!normalMap &&
            TexFile.TryGetRawDxt(bytes, out int width, out int height, out int format,
                                 out bool mipmapped, out byte[] raw))
        {
            Image.Format gpu = format switch
            {
                TexFile.FormatDxt3 => Image.Format.Dxt3,
                TexFile.FormatDxt5 => Image.Format.Dxt5,
                _ => Image.Format.Dxt1,
            };

            return new CpuLayer(width, height, mipmapped, gpu, raw);
        }

        if (!TexFile.TryDecode(bytes, MaxLayerTexels, out TexFile tex, out _) || tex.Levels.Length == 0)
        {
            return null;
        }

        if (tex.IsNormalMap)
        {
            foreach (byte[] level in tex.Levels)
            {
                for (int i = 0; i + 3 < level.Length; i += 4)
                {
                    (level[i], level[i + 3]) = (level[i + 3], level[i]);
                }
            }
        }

        int total = 0;
        foreach (byte[] level in tex.Levels)
        {
            total += level.Length;
        }

        var data = new byte[total];
        int at = 0;
        foreach (byte[] level in tex.Levels)
        {
            Buffer.BlockCopy(level, 0, data, at, level.Length);
            at += level.Length;
        }

        return new CpuLayer(tex.Width, tex.Height, tex.Levels.Length > 1, Image.Format.Rgba8, data);
    }

    private static readonly object FallbackGate = new();

    private static ImageTexture FallbackWhite
    {
        get
        {
            lock (FallbackGate)
            {
                if (_fallbackWhite is null)
                {
                    Image image = Image.CreateFromData(1, 1, false, Image.Format.Rgba8,
                                                       new byte[] { 255, 255, 255, 255 });
                    _fallbackWhite = ImageTexture.CreateFromImage(image);
                }

                return _fallbackWhite;
            }
        }
    }

    private static ImageTexture FallbackNormal
    {
        get
        {
            lock (FallbackGate)
            {
                if (_fallbackNormal is null)
                {
                    Image image = Image.CreateFromData(1, 1, false, Image.Format.Rgba8,
                                                       new byte[] { 128, 128, 255, 255 });
                    _fallbackNormal = ImageTexture.CreateFromImage(image);
                }

                return _fallbackNormal;
            }
        }
    }

    public static TileMaps BuildTileMaps(AreaTileCompute? tile)
    {
        var sw = System.Diagnostics.Stopwatch.StartNew();
        var byIndex = new AreaChunkCompute?[AreaChunk.ChunkCount];
        bool anyRawBlend = false, anyRawColour = false;
        foreach (AreaChunkCompute chunk in tile?.Chunks ?? Enumerable.Empty<AreaChunkCompute>())
        {
            if (chunk.Chunk.Index >= 0 && chunk.Chunk.Index < AreaChunk.ChunkCount)
            {
                byIndex[chunk.Chunk.Index] = chunk;
            }

            anyRawBlend |= chunk.BlendMap is not null;
            anyRawColour |= chunk.ColourMap is not null;
        }

        var maps = new TileMaps { BlendDxt = !anyRawBlend, ColourDxt = !anyRawColour };
        var blendImages = new Godot.Collections.Array<Image>();
        var colourImages = new Godot.Collections.Array<Image>();
        for (int i = 0; i < AreaChunk.ChunkCount; i++)
        {
            AreaChunkCompute? chunk = byIndex[i];
            blendImages.Add(BlendImage(chunk, maps.BlendDxt));
            colourImages.Add(ColourImage(chunk, maps.ColourDxt));
        }

        maps.Blend = new Texture2DArray();
        maps.Blend.CreateFromImages(blendImages);
        maps.Colour = new Texture2DArray();
        maps.Colour.CreateFromImages(colourImages);
        MapTicks += sw.ElapsedTicks;
        TileMapsBuilt++;
        return maps;
    }

    private static Image BlendImage(AreaChunkCompute? chunk, bool dxt)
    {
        if (dxt)
        {
            byte[] data = chunk?.BlendMapDxt ?? DefaultBlendDxt;
            return Image.CreateFromData(DxtTexels, DxtTexels, false, Image.Format.Dxt1, data);
        }

        byte[] rgba;
        if (chunk?.BlendMap is not null)
        {
            rgba = chunk.BlendMap;
        }
        else if (chunk?.BlendMapDxt is not null)
        {
            rgba = Crop(TexRaw.DecodeLevel(chunk.BlendMapDxt, 0, chunk.BlendMapDxt.Length, DxtTexels, DxtTexels, TexFile.FormatDxt1));
            for (int i = 0; i < rgba.Length; i += 4)
            {
                int sum = rgba[i] + rgba[i + 1] + rgba[i + 2];
                rgba[i + 3] = (byte)Math.Max(0, 255 - sum);
            }
        }
        else
        {
            rgba = new byte[MapTexels * MapTexels * 4];
            for (int i = 0; i < rgba.Length; i += 4)
            {
                rgba[i] = 255;
            }
        }

        return Image.CreateFromData(MapTexels, MapTexels, false, Image.Format.Rgba8, rgba);
    }

    private static Image ColourImage(AreaChunkCompute? chunk, bool dxt)
    {
        if (dxt)
        {
            byte[] data = chunk?.ColourMapDxt ?? DefaultColourDxt;
            return Image.CreateFromData(DxtTexels, DxtTexels, false, Image.Format.Dxt5, data);
        }

        byte[] rgba;
        if (chunk?.ColourMap is not null)
        {
            rgba = chunk.ColourMap;
        }
        else if (chunk?.ColourMapDxt is not null)
        {
            rgba = Crop(TexRaw.DecodeLevel(chunk.ColourMapDxt, 0, chunk.ColourMapDxt.Length, DxtTexels, DxtTexels, TexFile.FormatDxt5));
        }
        else
        {
            rgba = new byte[MapTexels * MapTexels * 4];
            for (int i = 0; i < rgba.Length; i += 4)
            {
                rgba[i] = 128;
                rgba[i + 1] = 128;
                rgba[i + 2] = 128;
                rgba[i + 3] = 255;
            }
        }

        return Image.CreateFromData(MapTexels, MapTexels, false, Image.Format.Rgba8, rgba);
    }

    private static byte[] Crop(byte[] rgba68)
    {
        var rgba = new byte[MapTexels * MapTexels * 4];
        for (int y = 0; y < MapTexels; y++)
        {
            Buffer.BlockCopy(rgba68, y * DxtTexels * 4, rgba, y * MapTexels * 4, MapTexels * 4);
        }

        return rgba;
    }

    private static byte[] DefaultBlendDxt
    {
        get
        {
            if (_defaultBlendDxt is null)
            {
                var data = new byte[BlendDxtBytes];
                for (int block = 0; block < BlendDxtBytes / 8; block++)
                {
                    data[block * 8 + 0] = 0x00;
                    data[block * 8 + 1] = 0xF8;
                    data[block * 8 + 2] = 0x00;
                    data[block * 8 + 3] = 0xF8;
                }

                _defaultBlendDxt = data;
            }

            return _defaultBlendDxt;
        }
    }

    private static byte[] DefaultColourDxt
    {
        get
        {
            if (_defaultColourDxt is null)
            {
                var data = new byte[ColourDxtBytes];
                for (int block = 0; block < ColourDxtBytes / 16; block++)
                {
                    data[block * 16 + 0] = 0xFF;
                    data[block * 16 + 1] = 0xFF;
                    data[block * 16 + 8] = 0x10;
                    data[block * 16 + 9] = 0x84;
                    data[block * 16 + 10] = 0x10;
                    data[block * 16 + 11] = 0x84;
                }

                _defaultColourDxt = data;
            }

            return _defaultColourDxt;
        }
    }

    public static ShaderMaterial BuildMaterial(uint[] worldLayerIds, TileMaps maps)
    {
        MaterialsBuilt++;
        var material = new ShaderMaterial { Shader = Shader };
        material.SetShaderParameter("blend_maps", maps.Blend);
        material.SetShaderParameter("colour_maps", maps.Colour);
        material.SetShaderParameter("blend_uv_scale", maps.BlendDxt ? (float)MapTexels / DxtTexels : 1.0f);
        material.SetShaderParameter("colour_uv_scale", maps.ColourDxt ? (float)MapTexels / DxtTexels : 1.0f);
        material.SetShaderParameter("blend_alpha_from_rgb", maps.BlendDxt ? 1.0f : 0.0f);

        var scale = new float[4];
        for (int i = 0; i < 4; i++)
        {
            uint layerId = i < worldLayerIds.Length ? worldLayerIds[i] : 0u;
            float layerScale;
            if (layerId == 0)
            {
                material.SetShaderParameter("layer" + i, FallbackWhite);
                material.SetShaderParameter("layer" + i + "_normal", FallbackNormal);
                layerScale = 4.0f;
            }
            else
            {
                CachedTexture cached = GetLayerTexture(layerId);
                material.SetShaderParameter("layer" + i, cached.Diffuse);
                material.SetShaderParameter("layer" + i + "_normal", cached.Normal);
                layerScale = AreaTables.TryWorldLayer(layerId, out WorldLayerEntry entry) ? entry.ScaleU : 4.0f;
            }

            scale[i] = layerScale > 0.0f ? 32.0f / layerScale : 8.0f;
        }

        material.SetShaderParameter("tex_scale", new Vector4(scale[0], scale[1], scale[2], scale[3]));
        material.SetShaderParameter("base_color", new Vector4(1.0f, 1.0f, 1.0f, 1.0f));
        material.SetShaderParameter("highlight_color", new Vector4(0.0f, 0.0f, 0.0f, 0.0f));
        return material;
    }

    private static readonly System.Collections.Concurrent.ConcurrentDictionary<string, ImageTexture?>
        Composites = new(StringComparer.OrdinalIgnoreCase);

    public static Texture2D? Composite(string mapName, AreaTileCoord tile)
    {
        string path = $"Map/{mapName}/{mapName}.{tile.Z:x2}{tile.X:x2}.tex";
        if (Composites.TryGetValue(path, out ImageTexture? cached))
        {
            return cached;
        }

        ImageTexture? texture = null;
        byte[]? bytes = AreaTables.Read(path);
        if (bytes is not null &&
            TexFile.TryGetRawDxt(bytes, out int width, out int height, out int format,
                                 out bool mipmapped, out byte[] data))
        {
            Image.Format gpu = format switch
            {
                TexFile.FormatDxt3 => Image.Format.Dxt3,
                TexFile.FormatDxt5 => Image.Format.Dxt5,
                _ => Image.Format.Dxt1,
            };

            texture = ImageTexture.CreateFromImage(
                Image.CreateFromData(width, height, mipmapped, gpu, data));
        }

        Composites[path] = texture;
        return texture;
    }

    public static ShaderMaterial BuildLowMaterial(Texture2D? composite)
    {
        MaterialsBuilt++;
        var material = new ShaderMaterial { Shader = LowShader };
        material.SetShaderParameter("composite", composite ?? (Texture2D)FallbackWhite);
        material.SetShaderParameter("base_color", new Vector4(1.0f, 1.0f, 1.0f, 1.0f));
        return material;
    }

    public static (uint, uint, uint, uint) Signature(AreaChunkCompute chunk) =>
        (chunk.WorldLayerIds[0], chunk.WorldLayerIds[1], chunk.WorldLayerIds[2], chunk.WorldLayerIds[3]);
}
