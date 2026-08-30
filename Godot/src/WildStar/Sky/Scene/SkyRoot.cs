using System;
using System.Collections.Generic;
using Godot;
using WildStar.Model;

namespace WildStar.Sky;

[Tool]
[GlobalClass]
public partial class SkyRoot : Node3D
{
    public const string EnvironmentNode = "Environment";
    public const string SunNode = "Sun";
    public const string ModelsNode = "Models";
    public const string ParticulatesNode = "Particulates";
    public const string ModelMeta = "sky_model";
    public const string GlareMeta = "sky_glare";
    public const string AlbedoMeta = "sky_albedo";

    public const float DefaultDayLengthSeconds = 1000.0f;

    private float _timeOfDay = 43200.0f;
    private float _sunEnergy = 1.0f;
    private float _modelScale = 10000.0f;
    private SkyFile? _sky;
    private byte[]? _parsedFrom;
    private List<(SkyFile Sky, float Weight)>? _blend;
    private SkyState? _state;
    private static Func<string, byte[]?>? _lutResolver;
    private static readonly Dictionary<string, byte[]?> LutCache = new(StringComparer.OrdinalIgnoreCase);
    private string _lutKey = string.Empty;

    [Export] public byte[] SkyData { get; set; } = Array.Empty<byte>();

    [Export] public string SourcePath { get; set; } = string.Empty;

    [Export(PropertyHint.Range, "0,86400,1")]
    public float TimeOfDay
    {
        get => _timeOfDay;
        set
        {
            _timeOfDay = ((value % 86400.0f) + 86400.0f) % 86400.0f;
            Apply();
        }
    }

    [Export] public bool RunClock { get; set; }

    [Export] public float DayLengthSeconds { get; set; } = DefaultDayLengthSeconds;

    [Export]
    public float SunEnergy
    {
        get => _sunEnergy;
        set
        {
            _sunEnergy = value;
            Apply();
        }
    }

    [Export]
    public float ModelScale
    {
        get => _modelScale;
        set
        {
            _modelScale = value;
            ApplyModelScale();
        }
    }

    private float _modelWeight = 1.0f;
    private bool _environmentEnabled = true;
    private Godot.Environment? _parkedEnvironment;
    private Compositor? _parkedCompositor;

    [Export(PropertyHint.Range, "0,1,0.001")]
    public float ModelWeight
    {
        get => _modelWeight;
        set
        {
            _modelWeight = Mathf.Clamp(value, 0.0f, 1.0f);
            Apply();
        }
    }

    [Export]
    public bool EnvironmentEnabled
    {
        get => _environmentEnabled;
        set
        {
            _environmentEnabled = value;
            Apply();
        }
    }

    [Export] public bool FogPass { get; set; } = true;

    [Export] public bool ColourGrade { get; set; } = true;

    private void ApplyModelScale()
    {
        var models = GetNodeOrNull<Node3D>(ModelsNode);
        if (models is not null)
        {
            models.Scale = Vector3.One * Mathf.Max(_modelScale, 0.0001f);
        }
    }

    public SkyFile? Sky
    {
        get
        {
            if (_sky is not null && ReferenceEquals(_parsedFrom, SkyData))
            {
                return _sky;
            }

            if (SkyData.Length == 0 || !SkyFile.TryParse(SkyData, out SkyFile sky, out _))
            {
                return null;
            }

            _sky = sky;
            _parsedFrom = SkyData;
            return _sky;
        }
    }

    public SkyState? State => _state;

    [Export] public bool FollowCamera { get; set; } = true;

    private double _sinceFollow;

    public static void SetLutResolver(Func<string, byte[]?>? resolver) => _lutResolver = resolver;

    public override void _Ready()
    {
        Apply();
        SetProcess(true);
    }

    public override void _Process(double delta)
    {
        if (RunClock && DayLengthSeconds > 0.0f)
        {
            TimeOfDay = _timeOfDay + (float)delta * (SkyFile.SecondsPerDay / DayLengthSeconds);
        }

        _sinceFollow += delta;
        if (_sinceFollow < 0.25 || !FollowCamera)
        {
            return;
        }

        _sinceFollow = 0;
        Camera3D? camera = CurrentCamera();
        if (camera is null)
        {
            return;
        }

        float fit = Mathf.Min(10000.0f, camera.Far * 0.8f);
        if (fit > 1.0f && Mathf.Abs(fit - ModelScale) > ModelScale * 0.05f)
        {
            ModelScale = fit;
        }

        Vector3 at = camera.GlobalPosition;
        Vector3 centre = GlobalPosition;
        float dx = at.X - centre.X;
        float dz = at.Z - centre.Z;
        if (Mathf.Sqrt(dx * dx + dz * dz) > ModelScale * 0.25f)
        {
            GlobalPosition = new Vector3(at.X, centre.Y, at.Z);
        }
    }

    private Camera3D? CurrentCamera()
    {
#if TOOLS
        if (Engine.IsEditorHint())
        {
            return EditorInterface.Singleton.GetEditorViewport3D(0)?.GetCamera3D();
        }
#endif
        return GetViewport()?.GetCamera3D();
    }

    public void SetBlend(IReadOnlyList<(SkyFile Sky, float Weight)>? sources)
    {
        _blend = sources is null || sources.Count == 0 ? null : new List<(SkyFile, float)>(sources);
        Apply();
    }

    public IReadOnlyList<(SkyFile Sky, float Weight)>? Blend => _blend;

    public void Drive(float timeOfDay, float modelWeight, bool environmentOwner,
                      IReadOnlyList<(SkyFile Sky, float Weight)>? blend)
    {
        RunClock = false;
        _timeOfDay = ((timeOfDay % 86400.0f) + 86400.0f) % 86400.0f;
        _modelWeight = Mathf.Clamp(modelWeight, 0.0f, 1.0f);
        _environmentEnabled = environmentOwner;
        _blend = environmentOwner && blend is not null && blend.Count > 1 ? new List<(SkyFile, float)>(blend) : null;
        Apply();
    }

    public void Apply()
    {
        SkyFile? sky = Sky;
        if (sky is null)
        {
            return;
        }

        uint time = (uint)Mathf.Clamp(_timeOfDay, 0.0f, 86399.0f);
        ApplyOwnership();
        if (_environmentEnabled)
        {
            SkyState state = _blend is null ? SkyState.Sample(sky, time) : SkyState.Blend(_blend, time);
            _state = state;
            ApplyEnvironment(state);
            ApplySun(state);
        }

        ApplyModels(sky, time);
        ApplyModelScale();
    }

    private void ApplyOwnership()
    {
        var world = GetNodeOrNull<WorldEnvironment>(EnvironmentNode);
        if (world is not null)
        {
            if (_environmentEnabled)
            {
                if (world.Environment is null && _parkedEnvironment is not null)
                {
                    world.Environment = _parkedEnvironment;
                    world.Compositor = _parkedCompositor;
                    _parkedEnvironment = null;
                    _parkedCompositor = null;
                }
            }
            else if (world.Environment is not null)
            {
                _parkedEnvironment = world.Environment;
                _parkedCompositor = world.Compositor;
                world.Environment = null;
                world.Compositor = null;
            }
        }

        var sun = GetNodeOrNull<DirectionalLight3D>(SunNode);
        if (sun is not null && !_environmentEnabled)
        {
            sun.Visible = false;
        }
    }

    private void ApplyEnvironment(SkyState state)
    {
        var world = GetNodeOrNull<WorldEnvironment>(EnvironmentNode);
        if (world?.Environment is null)
        {
            return;
        }

        Godot.Environment environment = world.Environment;
        float[] irradiance = SkyLighting.Irradiance(state.Sh);
        (Color ambientColour, float ambientEnergy) = HdrColour(irradiance[0], irradiance[1], irradiance[2]);
        environment.AmbientLightSource = Godot.Environment.AmbientSource.Color;
        environment.AmbientLightColor = ambientColour;
        environment.AmbientLightEnergy = ambientEnergy;
        environment.TonemapMode = Godot.Environment.ToneMapper.Linear;
        environment.TonemapExposure = 1.0f;
        environment.FogEnabled = false;

        if (environment.Sky?.SkyMaterial is ShaderMaterial material)
        {
            var bands = new Color[SkyState.BandCount];
            for (int i = 0; i < SkyState.BandCount; i++)
            {
                bands[i] = new Color(state.SkyBands[4 * i], state.SkyBands[4 * i + 1], state.SkyBands[4 * i + 2],
                                     state.SkyBands[4 * i + 3]);
            }

            material.SetShaderParameter("sky_sh", ShVectors(state.SkySh));
            material.SetShaderParameter("sky_bands", bands);
            material.SetShaderParameter("dome_weight", state.SkyGradientWeight);
            material.SetShaderParameter("dome_yaw", state.DomeYaw);
            material.SetShaderParameter("dome_pitch", state.DomePitch);
        }

        if (world.Compositor is Compositor compositor)
        {
            foreach (CompositorEffect? effect in compositor.CompositorEffects)
            {
                switch (effect)
                {
                    case SkyFogEffect fog:
                        PublishFog(fog, state);
                        break;
                    case SkyGradeEffect grade:
                        PublishGrade(grade, state);
                        break;
                }
            }
        }
    }

    private void PublishFog(SkyFogEffect fog, SkyState state)
    {
        var block = new float[SkyFogEffect.PublishedLength];
        block[0] = state.FogStart;
        block[1] = state.FogEnd;
        block[2] = FogPass ? 1.0f : 0.0f;
        block[3] = state.FogGradientWeight;
        for (int i = 0; i < 9; i++)
        {
            block[4 + 4 * i] = state.FogSh[i];
            block[4 + 4 * i + 1] = state.FogSh[9 + i];
            block[4 + 4 * i + 2] = state.FogSh[18 + i];
        }

        Array.Copy(state.FogBands, 0, block, 40, 64);
        fog.Enabled = FogPass;
        fog.Publish(block);
    }

    private void PublishGrade(SkyGradeEffect grade, SkyState state)
    {
        bool active = ColourGrade && state.HasColourGrading;
        var block = new float[SkyGradeEffect.PublishedLength];
        block[0] = active ? 1.0f : 0.0f;
        block[1] = state.Luts.Count > 0 ? 1.0f : 0.0f;
        block[2] = 1.0f;
        block[4] = state.Post[0];
        block[5] = state.Post[1];
        block[6] = state.Post[2];
        block[7] = state.Post[3];
        block[8] = state.Post[6];
        block[9] = state.Post[7];
        block[10] = state.Post[8];
        block[11] = state.Post[9];
        grade.Enabled = active;
        grade.Publish(block);

        string key = LutKey(state);
        if (!string.Equals(key, _lutKey, StringComparison.Ordinal))
        {
            _lutKey = key;
            grade.PublishLut(BlendLuts(state));
        }
    }

    private static string LutKey(SkyState state)
    {
        if (state.Luts.Count == 0)
        {
            return string.Empty;
        }

        var parts = new List<string>(state.Luts.Count);
        foreach ((string path, float weight) in state.Luts)
        {
            parts.Add(path + ":" + weight.ToString("0.000", System.Globalization.CultureInfo.InvariantCulture));
        }

        return string.Join("|", parts);
    }

    private static byte[]? BlendLuts(SkyState state)
    {
        if (state.Luts.Count == 0)
        {
            return null;
        }

        const int count = SkyGradeEffect.LutSize * SkyGradeEffect.LutSize * SkyGradeEffect.LutSize;
        var sum = new float[count * 4];
        float covered = 0.0f;
        foreach ((string path, float weight) in state.Luts)
        {
            byte[]? lut = LoadLut(path);
            if (lut is null)
            {
                continue;
            }

            covered += weight;
            for (int i = 0; i < sum.Length; i++)
            {
                sum[i] += lut[i] * weight;
            }
        }

        if (covered <= 0.0f)
        {
            return null;
        }

        byte[] identity = SkyGradeEffect.IdentityLut();
        float remainder = Mathf.Max(0.0f, 1.0f - covered);
        var result = new byte[sum.Length];
        for (int i = 0; i < sum.Length; i++)
        {
            result[i] = (byte)Mathf.Clamp(Mathf.RoundToInt(sum[i] + identity[i] * remainder), 0, 255);
        }

        return result;
    }

    public static byte[]? LoadLut(string path)
    {
        lock (LutCache)
        {
            if (LutCache.TryGetValue(path, out byte[]? cached))
            {
                return cached;
            }
        }

        byte[]? decoded = null;
        byte[]? bytes = _lutResolver?.Invoke(path.Replace('\\', '/'));
        const int count = SkyGradeEffect.LutSize * SkyGradeEffect.LutSize * SkyGradeEffect.LutSize;
        if (bytes is not null && bytes.Length >= WildStar.Texture.TexFile.DataStart + count * 4)
        {
            decoded = new byte[count * 4];
            int src = WildStar.Texture.TexFile.DataStart;
            for (int i = 0; i < count; i++)
            {
                decoded[4 * i] = bytes[src + 4 * i + 2];
                decoded[4 * i + 1] = bytes[src + 4 * i + 1];
                decoded[4 * i + 2] = bytes[src + 4 * i];
                decoded[4 * i + 3] = 255;
            }
        }
        else if (bytes is null)
        {
            GD.PushWarning("[wildstar_mount] sky LUT not found: " + path);
        }

        lock (LutCache)
        {
            LutCache[path] = decoded;
        }

        return decoded;
    }

    private static Vector3[] ShVectors(float[] sh)
    {
        var coefficients = new Vector3[9];
        for (int i = 0; i < 9; i++)
        {
            coefficients[i] = new Vector3(sh[i], sh[9 + i], sh[18 + i]);
        }

        return coefficients;
    }

    private void ApplySun(SkyState state)
    {
        var sun = GetNodeOrNull<DirectionalLight3D>(SunNode);
        if (sun is null)
        {
            return;
        }

        if (!state.HasSun)
        {
            sun.Visible = false;
            return;
        }

        var toSun = new Vector3(state.SunDirection[0], state.SunDirection[1], state.SunDirection[2]);
        sun.Visible = true;
        (Color sunColour, float sunEnergyScale) = HdrColour(state.SunColour[0], state.SunColour[1], state.SunColour[2]);
        sun.LightColor = sunColour;
        sun.LightEnergy = _sunEnergy * sunEnergyScale;
        if (toSun.LengthSquared() > 0.000001f)
        {
            Vector3 up = Mathf.Abs(toSun.Normalized().Dot(Vector3.Up)) > 0.999f ? Vector3.Forward : Vector3.Up;
            sun.Transform = new Transform3D(Basis.LookingAt(-toSun, up), Vector3.Zero);
        }
    }

    private void ApplyParticulates()
    {
        var particulates = GetNodeOrNull<Node3D>(ParticulatesNode);
        if (particulates is null)
        {
            return;
        }

        bool visible = _modelWeight > 0.0f;
        foreach (Node child in particulates.GetChildren())
        {
            if (child is not M3ParticleNode node)
            {
                continue;
            }

            node.Alpha = _modelWeight;
            if (node.Visible != visible)
            {
                node.Visible = visible;
            }
        }
    }

    private void ApplyModels(SkyFile sky, uint time)
    {
        ApplyParticulates();
        var models = GetNodeOrNull<Node3D>(ModelsNode);
        if (models is null)
        {
            return;
        }

        SkyColour glare = SkyColour.Sample(sky.GlareColour, time, 0, SkyColour.White);
        SkyModelValue? sunValue = null;
        int sunIndex = sky.SunModelIndex;
        if (sunIndex >= 0 && sky.Models[sunIndex].TrySample(time, out SkyModelValue sv))
        {
            sunValue = sv;
        }

        foreach (Node child in models.GetChildren())
        {
            if (child is not Node3D node)
            {
                continue;
            }

            if (node.HasMeta(ModelMeta))
            {
                int index = node.GetMeta(ModelMeta).AsInt32();
                if (index < 0 || index >= sky.Models.Length)
                {
                    continue;
                }

                SkyModel record = sky.Models[index];
                if (!record.TrySample(time, out SkyModelValue value))
                {
                    node.Visible = false;
                    continue;
                }

                node.Visible = _modelWeight > 0.0f;
                node.Transform = new Transform3D(RotationOf(value.Direction), Vector3.Zero);
                Tint(node, value.Tint, _modelWeight);
                if (record.AnimatesWithTimeOfDay)
                {
                    Animate(node, time);
                }
            }
            else if (node.HasMeta(GlareMeta))
            {
                if (sunValue is null)
                {
                    node.Visible = false;
                    continue;
                }

                node.Visible = _modelWeight > 0.0f;
                node.Transform = new Transform3D(RotationOf(sunValue.Value.Direction), Vector3.Zero);
                Tint(node, new SkyColour(glare.R, glare.G, glare.B, sunValue.Value.Tint.A), _modelWeight);
            }
        }
    }

    public static Basis RotationOf(SkyDirection direction)
    {
        float[] m = SkyLighting.ModelRotation(direction);
        return new Basis(new Vector3(m[0], m[1], m[2]), new Vector3(m[3], m[4], m[5]),
                         new Vector3(m[6], m[7], m[8]));
    }

    private static void Tint(Node3D node, SkyColour tint, float weight)
    {
        var stack = new Stack<Node>();
        stack.Push(node);
        while (stack.Count > 0)
        {
            Node current = stack.Pop();
            foreach (Node child in current.GetChildren())
            {
                stack.Push(child);
            }

            if (current is not MeshInstance3D instance || instance.Mesh is null)
            {
                continue;
            }

            instance.Transparency = Mathf.Clamp(1.0f - tint.A * weight, 0.0f, 1.0f);
            for (int s = 0; s < instance.Mesh.GetSurfaceCount(); s++)
            {
                if (instance.GetSurfaceOverrideMaterial(s) is not StandardMaterial3D material)
                {
                    if (instance.Mesh.SurfaceGetMaterial(s) is not StandardMaterial3D shared)
                    {
                        continue;
                    }

                    material = (StandardMaterial3D)shared.Duplicate();
                    instance.SetSurfaceOverrideMaterial(s, material);
                }

                if (!material.HasMeta(AlbedoMeta))
                {
                    material.SetMeta(AlbedoMeta, material.AlbedoColor);
                }

                Color albedo = material.GetMeta(AlbedoMeta).AsColor();
                material.AlbedoColor = new Color(albedo.R * tint.R, albedo.G * tint.G, albedo.B * tint.B, albedo.A);
            }
        }
    }

    private static void Animate(Node3D node, uint time)
    {
        var stack = new Stack<Node>();
        stack.Push(node);
        while (stack.Count > 0)
        {
            Node current = stack.Pop();
            foreach (Node child in current.GetChildren())
            {
                stack.Push(child);
            }

            if (current is not M3AnimatedSkeleton animator)
            {
                continue;
            }

            if (animator.Play(SkyFile.TimeOfDaySequence, 0))
            {
                animator.SeekNormalized(time / (double)SkyFile.SecondsPerDay);
                animator.Playing = false;
            }
        }
    }

    private static (Color Colour, float Energy) HdrColour(float r, float g, float b)
    {
        r = Mathf.Max(r, 0.0f);
        g = Mathf.Max(g, 0.0f);
        b = Mathf.Max(b, 0.0f);
        float peak = Mathf.Max(r, Mathf.Max(g, b));
        return peak > 1.0f
            ? (new Color(r / peak, g / peak, b / peak, 1.0f), peak)
            : (new Color(r, g, b, 1.0f), 1.0f);
    }
}
