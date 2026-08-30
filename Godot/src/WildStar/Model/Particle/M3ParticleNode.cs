using System;
using Godot;

namespace WildStar.Model;

[Tool]
public partial class M3ParticleNode : MultiMeshInstance3D
{
    private const int FloatsPerInstance = 12 + 4 + 4;

    private M3ParticleEmitter? _emitter;
    private M3ParticleSimulation? _simulation;
    private ShaderMaterial? _material;
    private float[] _buffer = Array.Empty<float>();
    private readonly float[] _alphaBake = new float[M3ParticleCurve.BakedSamples];
    private readonly float[] _sizeBake = new float[M3ParticleCurve.BakedSamples];
    private readonly Color[] _colourBake0 = new Color[M3ParticleCurve.BakedSamples];
    private readonly Color[] _colourBake1 = new Color[M3ParticleCurve.BakedSamples];

    [Export(PropertyHint.Range, "0,1,0.001")] public float Alpha { get; set; } = 1.0f;

    [Export] public Color Tint { get; set; } = Colors.White;

    [Export(PropertyHint.Range, "0,2,0.01")] public float Density { get; set; } = 1.0f;

    [Export] public bool FollowCamera { get; set; }

    [Export] public bool Simulate { get; set; } = true;

    public M3ParticleEmitter? Emitter => _emitter;

    public M3ParticleSimulation? Simulation => _simulation;

    public static M3ParticleNode? Create(M3File model, int emitterIndex, string modelPath, string name)
    {
        if (emitterIndex < 0 || emitterIndex >= model.ParticleEmitters.Length)
        {
            return null;
        }

        M3ParticleEmitter emitter = model.ParticleEmitters[emitterIndex];
        if (!emitter.HasBlock || emitter.Class != 0 || emitter.Kind == 1)
        {
            return null;
        }

        var node = new M3ParticleNode { Name = name };
        node.Bind(emitter, TextureFor(model, emitter.TextureIndex));
        return node;
    }

    public void Bind(M3ParticleEmitter emitter, Texture2D? texture)
    {
        _emitter = emitter;
        int seed = emitter.Seed != 0 ? (int)emitter.Seed : System.Environment.TickCount ^ GetHashCode();
        _simulation = new M3ParticleSimulation(emitter, seed);

        var shader = new Shader { Code = M3ParticleShaders.Sprite(emitter.BlendMode, emitter.DepthTest, emitter.DepthWrite) };
        _material = new ShaderMaterial { Shader = shader };
        if (texture is not null)
        {
            _material.SetShaderParameter("albedo", texture);
        }

        _material.SetShaderParameter("uv_scale", new Vector2(emitter.UvScaleX, emitter.UvScaleY));
        _material.SetShaderParameter("columns", (float)Math.Max(emitter.Columns, 1));

        Multimesh = new MultiMesh
        {
            TransformFormat = MultiMesh.TransformFormatEnum.Transform3D,
            UseColors = true,
            UseCustomData = true,
            Mesh = new QuadMesh { Size = Vector2.One, Material = _material },
            InstanceCount = 0,
        };

        CastShadow = ShadowCastingSetting.Off;
        IgnoreOcclusionCulling = true;
        float reach = M3ParticleSimulation.WrapHalf + 8.0f;
        CustomAabb = new Aabb(new Vector3(-reach, -reach, -reach), new Vector3(reach * 2.0f, reach * 2.0f, reach * 2.0f));
        SetProcess(true);
    }

    public override void _Ready()
    {
        SetProcess(true);
    }

    public override void _Process(double delta)
    {
        if (_simulation is null || _emitter is null || _material is null)
        {
            return;
        }

        if (FollowCamera)
        {
            Camera3D? camera = CurrentCamera();
            if (camera is not null)
            {
                GlobalPosition = camera.GlobalPosition;
            }
        }

        if (!Simulate || !IsVisibleInTree())
        {
            return;
        }

        _simulation.Update((float)delta, GlobalPosition, Basis.Identity, Density);
        Fill();
    }

    public void Restart()
    {
        _simulation?.Clear();
        if (Multimesh is not null)
        {
            Multimesh.InstanceCount = 0;
        }
    }

    private void Fill()
    {
        if (_simulation is null || _emitter is null || _material is null || Multimesh is null)
        {
            return;
        }

        M3ParticleEmitter e = _emitter;
        uint time = _simulation.TimeMs;
        float intensity = e.Intensity.Count == 0 ? 1.0f : M3ParticleTrack.Half(e.Intensity, time, 1.0f);
        _material.SetShaderParameter("tint", new Color(Tint.R * intensity, Tint.G * intensity, Tint.B * intensity, 1.0f));

        Bake(time);

        ReadOnlySpan<M3ParticleSimulation.Particle> particles = _simulation.Particles;
        ReadOnlySpan<M3ParticleSimulation.Variant> variants = _simulation.Variants;
        int count = particles.Length;
        if (count == 0)
        {
            Multimesh.InstanceCount = 0;
            return;
        }

        if (_buffer.Length < count * FloatsPerInstance)
        {
            _buffer = new float[Math.Max(count, 64) * FloatsPerInstance];
        }

        float frameTime = Math.Max(e.FrameTimeMs, 1);
        float frames = Math.Max(e.FrameCount, 1);
        float lastFrame = Math.Max(e.FrameCount - 1, 0);
        bool loop = e.LoopFrames;

        int o = 0;
        for (int i = 0; i < count; i++)
        {
            ref readonly M3ParticleSimulation.Particle p = ref particles[i];
            ref readonly M3ParticleSimulation.Variant v = ref variants[p.Variant];
            int ageMs = p.LifeMs - p.RemainingMs;
            float t = p.LifeMs > 0 ? MathF.Min(ageMs / (float)p.LifeMs, 0.999f) : 0.999f;
            float ageSeconds = ageMs * 0.001f;
            float angle = v.Rotation + v.Spin * ageSeconds + v.SpinAcceleration * (ageSeconds * ageSeconds * 0.5f);
            float size = Baked(_sizeBake, t) * p.SizeRandom;
            float alpha = Baked(_alphaBake, t) * Alpha;
            Color colour = Baked(_colourBake0, t).Lerp(Baked(_colourBake1, t), v.ColourBlend).SrgbToLinear();

            float frame = MathF.Floor(ageMs / frameTime) + v.FrameOffset;
            frame = loop ? Mod(frame, frames) : MathF.Min(frame, lastFrame);
            float signs = (v.SignU > 0.0f ? 1.0f : 0.0f) + (v.SignV > 0.0f ? 2.0f : 0.0f);

            _buffer[o + 0] = 1.0f;
            _buffer[o + 1] = 0.0f;
            _buffer[o + 2] = 0.0f;
            _buffer[o + 3] = p.Position.X;
            _buffer[o + 4] = 0.0f;
            _buffer[o + 5] = 1.0f;
            _buffer[o + 6] = 0.0f;
            _buffer[o + 7] = p.Position.Y;
            _buffer[o + 8] = 0.0f;
            _buffer[o + 9] = 0.0f;
            _buffer[o + 10] = 1.0f;
            _buffer[o + 11] = p.Position.Z;
            _buffer[o + 12] = colour.R;
            _buffer[o + 13] = colour.G;
            _buffer[o + 14] = colour.B;
            _buffer[o + 15] = alpha;
            _buffer[o + 16] = size;
            _buffer[o + 17] = angle;
            _buffer[o + 18] = frame;
            _buffer[o + 19] = signs;
            o += FloatsPerInstance;
        }

        if (Multimesh.InstanceCount != count)
        {
            Multimesh.InstanceCount = count;
        }

        Multimesh.Buffer = _buffer.AsSpan(0, count * FloatsPerInstance).ToArray();
    }

    private void Bake(uint time)
    {
        M3ParticleEmitter e = _emitter!;
        for (int i = 0; i < M3ParticleCurve.BakedSamples; i++)
        {
            float t = i / (float)(M3ParticleCurve.BakedSamples - 1);
            _alphaBake[i] = e.Alpha.Sample(t, time, 1.0f);
            _sizeBake[i] = e.Size.Sample(t, time, 1.0f);
            _colourBake0[i] = ToColour(e.Colour.Length > 0 ? e.Colour[0].SampleColour(t, time) : (1.0f, 1.0f, 1.0f));
            _colourBake1[i] = ToColour(e.Colour.Length > 1 ? e.Colour[1].SampleColour(t, time) : (1.0f, 1.0f, 1.0f));
        }
    }

    private static float Baked(float[] samples, float t)
    {
        float scaled = t * (M3ParticleCurve.BakedSamples - 1);
        int index = Math.Clamp((int)MathF.Floor(scaled), 0, M3ParticleCurve.BakedSamples - 2);
        float frac = scaled - index;
        return samples[index] + (samples[index + 1] - samples[index]) * frac;
    }

    private static Color Baked(Color[] samples, float t)
    {
        float scaled = t * (M3ParticleCurve.BakedSamples - 1);
        int index = Math.Clamp((int)MathF.Floor(scaled), 0, M3ParticleCurve.BakedSamples - 2);
        float frac = scaled - index;
        return samples[index].Lerp(samples[index + 1], frac);
    }

    private static Color ToColour((float R, float G, float B) c) =>
        new(Mathf.Clamp(c.R, 0.0f, 1.0f), Mathf.Clamp(c.G, 0.0f, 1.0f), Mathf.Clamp(c.B, 0.0f, 1.0f), 1.0f);

    private static float Mod(float value, float divisor)
    {
        float r = value % divisor;
        return r < 0.0f ? r + divisor : r;
    }

    private static Texture2D? TextureFor(M3File model, int index)
    {
        if (index < 0 || index >= model.Textures.Length)
        {
            return null;
        }

        string path = model.Textures[index].Path;
        return path.Length == 0 ? null : M3TextureCache.Get(path);
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
}
