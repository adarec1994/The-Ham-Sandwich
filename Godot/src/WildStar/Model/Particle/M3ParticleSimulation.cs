using System;
using Godot;

namespace WildStar.Model;

public sealed class M3ParticleSimulation
{
    public const int ParticleCap = 1024;
    public const int MaxFrameMs = 100;
    public const float WrapHalf = 10.0f;
    public const float WrapSize = 20.0f;

    public struct Particle
    {
        public Vector3 Position;
        public Vector3 Velocity;
        public int LifeMs;
        public int RemainingMs;
        public float SizeRandom;
        public int Variant;
    }

    public struct Variant
    {
        public float Rotation;
        public float Spin;
        public float SpinAcceleration;
        public float ColourBlend;
        public int FrameOffset;
        public float SignU;
        public float SignV;
    }

    private readonly M3ParticleEmitter _emitter;
    private readonly Random _random;
    private readonly Variant[] _variants = new Variant[M3ParticleEmitter.VariantCount];
    private Particle[] _particles = new Particle[64];
    private int _count;
    private float _spawnAccumulator;
    private ushort _counter;
    private uint _timeMs;
    private bool _hasPrevious;
    private Vector3 _previousOrigin;

    private Vector3 _drift;
    private Vector3 _driftFrom;
    private Vector3 _driftTo;
    private uint _phaseStart;
    private uint _phaseEnd;
    private bool _holdPhase;

    public M3ParticleSimulation(M3ParticleEmitter emitter, int seed)
    {
        _emitter = emitter;
        _random = new Random(seed);
        for (int i = 0; i < _variants.Length; i++)
        {
            ref Variant v = ref _variants[i];
            v.Rotation = Range(emitter.RotationMin, emitter.RotationMax);
            v.Spin = Range(emitter.SpinMin, emitter.SpinMax);
            v.SpinAcceleration = Range(emitter.SpinAccelerationMin, emitter.SpinAccelerationMax);
            if (emitter.RandomSpinSign && _random.Next(2) == 1)
            {
                v.Rotation = -v.Rotation;
                v.Spin = -v.Spin;
                v.SpinAcceleration = -v.SpinAcceleration;
            }

            v.ColourBlend = (float)_random.NextDouble();
            v.FrameOffset = RangeInt(emitter.FrameOffsetMin, emitter.FrameOffsetMax);
            v.SignU = emitter.FlipU && _random.Next(2) == 1 ? -1.0f : 1.0f;
            v.SignV = emitter.FlipV && _random.Next(2) == 1 ? -1.0f : 1.0f;
        }
    }

    public M3ParticleEmitter Emitter => _emitter;

    public int Count => _count;

    public ReadOnlySpan<Particle> Particles => _particles.AsSpan(0, _count);

    public ReadOnlySpan<Variant> Variants => _variants;

    public uint TimeMs => _timeMs;

    public Vector3 Drift => _drift;

    public void Update(float deltaSeconds, Vector3 origin, Basis basis, float density = 1.0f)
    {
        int frameMs = Math.Clamp((int)(deltaSeconds * 1000.0f), 0, MaxFrameMs);
        if (frameMs <= 0)
        {
            return;
        }

        M3ParticleEmitter e = _emitter;
        uint time = _timeMs;
        _timeMs += (uint)frameMs;
        if (!_hasPrevious)
        {
            _previousOrigin = origin;
            _hasPrevious = true;
        }

        Vector3 moved = origin - _previousOrigin;
        bool wrap = e.Kind == 2;

        if (wrap && _count > 0 && moved != Vector3.Zero)
        {
            for (int i = 0; i < _count; i++)
            {
                _particles[i].Position -= moved;
            }
        }

        if (wrap)
        {
            UpdateDrift(origin, basis, time);
        }

        Vector3 acceleration = basis * ToVector(M3ParticleTrack.Half3(e.Acceleration, time));
        float dt = frameMs * 0.001f;
        for (int i = _count - 1; i >= 0; i--)
        {
            ref Particle p = ref _particles[i];
            if (p.RemainingMs > 0)
            {
                Step(ref p, frameMs, dt, acceleration, wrap, time);
            }

            if (wrap)
            {
                p.Position = new Vector3(Wrap(p.Position.X), Wrap(p.Position.Y), Wrap(p.Position.Z));
            }

            if (p.RemainingMs <= 0)
            {
                _particles[i] = _particles[_count - 1];
                _count--;
            }
        }

        Spawn(frameMs, origin, basis, moved, time, density);
        _previousOrigin = origin;
    }

    public void Clear()
    {
        _count = 0;
        _spawnAccumulator = 0.0f;
    }

    private void Spawn(int frameMs, Vector3 origin, Basis basis, Vector3 moved, uint time, float density)
    {
        M3ParticleEmitter e = _emitter;
        bool enabled = e.Enable.Count == 0 || M3ParticleTrack.U8Step(e.Enable, time) != 0;
        if (!enabled)
        {
            return;
        }

        int interval = Math.Max(RangeInt(M3ParticleTrack.U32(e.IntervalMin, time), M3ParticleTrack.U32(e.IntervalMax, time)), 1);
        float bursts = frameMs / (float)interval * density;
        _spawnAccumulator += bursts;
        int whole = (int)MathF.Floor(_spawnAccumulator);
        _spawnAccumulator -= whole;
        if (whole <= 0)
        {
            return;
        }

        int perBurst = RangeInt(M3ParticleTrack.U16(e.SpawnCountMin, time), M3ParticleTrack.U16(e.SpawnCountMax, time));
        int count = whole * perBurst;
        int cap = Math.Min(Math.Max(M3ParticleTrack.U32(e.MaxParticles, time), 0), ParticleCap);
        if (_count + count > cap)
        {
            count = Math.Max(cap - _count, 0);
        }

        if (count <= 0)
        {
            return;
        }

        Ensure(_count + count);
        int lifeMin = M3ParticleTrack.U32(e.LifeMin, time);
        int lifeMax = M3ParticleTrack.U32(e.LifeMax, time);
        float sizeMin = M3ParticleTrack.Half(e.SizeRandomMin, time);
        float sizeMax = M3ParticleTrack.Half(e.SizeRandomMax, time);
        Vector3 trail = -moved / count;
        for (int i = 0; i < count; i++)
        {
            Vector3 offset = e.Kind == 2 ? trail * (count - i - 1) : Vector3.Zero;
            if (!SpawnOne(i, count, basis, offset, time, out Vector3 position, out Vector3 velocity))
            {
                continue;
            }

            ref Particle p = ref _particles[_count++];
            p.Position = position;
            p.Velocity = velocity;
            int life = Math.Min(RangeInt(lifeMin, lifeMax), 0x7FFF);
            p.LifeMs = life;
            p.RemainingMs = life;
            p.SizeRandom = MathF.Min(Range(sizeMin, sizeMax), 127.0f);
            p.Variant = _counter % M3ParticleEmitter.VariantCount;
            _counter++;
        }
    }

    private bool SpawnOne(int index, int total, Basis basis, Vector3 offset, uint time,
                          out Vector3 position, out Vector3 velocity)
    {
        M3ParticleEmitter e = _emitter;
        Vector3 local;
        switch (e.Shape)
        {
            case M3ParticleEmitter.ShapePoint:
                local = Vector3.Zero;
                break;
            case M3ParticleEmitter.ShapeLine:
            {
                float length = ShapeValue(0, time);
                float x = (e.ShapeFlags & 1) != 0
                    ? length * -0.5f + index * length / total
                    : Range(length * -0.5f, length * 0.5f);
                local = new Vector3(x, 0.0f, 0.0f);
                break;
            }
            case M3ParticleEmitter.ShapeRectangle:
            {
                float w = ShapeValue(0, time);
                float d = ShapeValue(1, time);
                local = new Vector3(Range(w * -0.5f, w * 0.5f), 0.0f, Range(d * -0.5f, d * 0.5f));
                break;
            }
            case M3ParticleEmitter.ShapeBox:
            {
                float w = ShapeValue(0, time);
                float h = ShapeValue(1, time);
                float d = ShapeValue(2, time);
                local = new Vector3(Range(w * -0.5f, w * 0.5f), Range(h * -0.5f, h * 0.5f), Range(d * -0.5f, d * 0.5f));
                break;
            }
            case M3ParticleEmitter.ShapeRing:
            {
                float radius = Range(ShapeValue(0, time), ShapeValue(1, time));
                float angle = (e.ShapeFlags & 1) != 0
                    ? ShapeValue(2, time) + index * (ShapeValue(3, time) - ShapeValue(2, time)) / total
                    : Range(ShapeValue(2, time), ShapeValue(3, time));
                local = new Vector3(MathF.Cos(angle) * radius, 0.0f, MathF.Sin(angle) * radius);
                break;
            }
            case M3ParticleEmitter.ShapeSphere:
            {
                Vector3 direction;
                do
                {
                    direction = new Vector3(Range(-1.0f, 1.0f), Range(-1.0f, 1.0f), Range(-1.0f, 1.0f));
                }
                while (direction == Vector3.Zero);

                local = direction.Normalized() * Range(ShapeValue(0, time), ShapeValue(1, time));
                break;
            }
            default:
                position = Vector3.Zero;
                velocity = Vector3.Zero;
                return false;
        }

        if (e.Kind == 2)
        {
            local = new Vector3(Range(-10.0f, 10.0f), Range(-10.0f, 10.0f), Range(-10.0f, 10.0f));
        }

        position = basis * local + offset;

        if (e.AttractToEmitter)
        {
            Vector3 toCentre = -position;
            float length = toCentre.Length();
            Vector3 direction = length > 0.0f ? toCentre / length : Vector3.Zero;
            velocity = direction * M3ParticleTrack.Half(e.VelocityAScale, time);
        }
        else
        {
            float spreadMin = M3ParticleTrack.Half(e.SpreadMin, time);
            float spreadMax = M3ParticleTrack.Half(e.Spread, time);
            float radialMin = M3ParticleTrack.Half(e.RadialSpreadMin, time);
            float radialMax = M3ParticleTrack.Half(e.RadialSpread, time);
            Vector3 velocityA = Cone(ToVector(M3ParticleTrack.Half3(e.VelocityA, time)), spreadMin, spreadMax);
            Vector3 radial = Cone(local, radialMin, radialMax);
            Vector3 combined = radial * M3ParticleTrack.Half(e.RadialScale, time) +
                               velocityA * M3ParticleTrack.Half(e.VelocityAScale, time);
            velocity = basis * combined + Cone(ToVector(M3ParticleTrack.Half3(e.VelocityB, time)), spreadMin, spreadMax);
        }

        velocity *= Range(M3ParticleTrack.Half(e.SpeedMin, time), M3ParticleTrack.Half(e.SpeedMax, time));
        return true;
    }

    private void Step(ref Particle p, int frameMs, float dt, Vector3 acceleration, bool wrap, uint time)
    {
        M3ParticleEmitter e = _emitter;
        float fraction = p.LifeMs > 0 ? (p.LifeMs - p.RemainingMs) / (float)p.LifeMs : 1.0f;
        float speed = e.Speed.Sample(fraction, time, 1.0f);
        p.Velocity += acceleration * dt;
        Vector3 motion = wrap ? (_drift + p.Velocity * speed) * dt : p.Velocity * speed * dt;
        p.Position += motion;
        p.RemainingMs -= frameMs;
    }

    private void UpdateDrift(Vector3 origin, Basis basis, uint time)
    {
        M3ParticleEmitter e = _emitter;
        uint now = _timeMs;
        if (now < _phaseEnd)
        {
            float span = _phaseEnd - _phaseStart;
            float t = span > 0.0f ? (now - _phaseStart) / span : 1.0f;
            _drift = _driftFrom + (_driftTo - _driftFrom) * t;
            return;
        }

        _phaseStart = now;
        if (_holdPhase)
        {
            _phaseEnd = now + (uint)Math.Max(RangeInt(e.HoldMinMs, e.HoldMaxMs), 0);
            _driftFrom = _driftTo;
            _drift = _driftTo;
        }
        else
        {
            _phaseEnd = now + (uint)Math.Max(RangeInt(e.MoveMinMs, e.MoveMaxMs), 0);
            _driftFrom = _driftTo;
            if (SpawnOne(0, 1, basis, Vector3.Zero, time, out _, out Vector3 target))
            {
                _driftTo = target;
            }

            _drift = _driftFrom;
        }

        _holdPhase = !_holdPhase;
    }

    private Vector3 Cone(Vector3 axis, float minAngle, float maxAngle)
    {
        float length = axis.Length();
        if (length <= 0.0f)
        {
            return Vector3.Zero;
        }

        Vector3 n = axis / length;
        Vector3 perpendicular = n.X != 0.0f || n.Z != 0.0f ? n.Cross(Vector3.Up).Normalized() : Vector3.Right;
        float polar = Range(minAngle, maxAngle);
        float azimuth = Range(0.0f, MathF.PI * 2.0f);
        Vector3 tilted = n.Rotated(perpendicular, polar);
        return tilted.Rotated(n, azimuth) * length;
    }

    private float ShapeValue(int index, uint time) =>
        index < _emitter.ShapeTracks.Length ? M3ParticleTrack.Half(_emitter.ShapeTracks[index], time) : 0.0f;

    private static float Wrap(float value)
    {
        if (value >= 0.0f)
        {
            return (value + WrapHalf) % WrapSize - WrapHalf;
        }

        return (value - WrapHalf) % WrapSize + WrapHalf;
    }

    private static Vector3 ToVector((float X, float Y, float Z) v) => new(v.X, v.Y, v.Z);

    private float Range(float min, float max) => min + (float)_random.NextDouble() * (max - min);

    private int RangeInt(int min, int max) => min + (int)(_random.NextDouble() * (max - min + 1.0));

    private void Ensure(int capacity)
    {
        if (_particles.Length >= capacity)
        {
            return;
        }

        int size = _particles.Length;
        while (size < capacity)
        {
            size *= 2;
        }

        Array.Resize(ref _particles, Math.Min(size, ParticleCap));
    }
}
