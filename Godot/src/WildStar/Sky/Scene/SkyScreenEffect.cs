using System;
using Godot;
using Godot.Collections;

namespace WildStar.Sky;

public abstract partial class SkyScreenEffect : CompositorEffect
{
    private RenderingDevice? _rd;
    private Rid _shader;
    private Rid _pipeline;
    private Rid _paramsBuffer;
    private int _paramsSize;
    private Rid _sampler;
    private volatile float[]? _params;

    protected SkyScreenEffect()
    {
        EffectCallbackType = EffectCallbackTypeEnum.PostTransparent;
        AccessResolvedColor = true;
        AccessResolvedDepth = true;
        _rd = RenderingServer.GetRenderingDevice();
        if (_rd is not null)
        {
            RenderingServer.CallOnRenderThread(Callable.From(Initialise));
        }
    }

    protected abstract string Source { get; }

    public void Publish(float[] parameters) => _params = parameters;

    public bool Ready => _pipeline.IsValid;

    private void Initialise()
    {
        if (_rd is null)
        {
            return;
        }

        var source = new RDShaderSource { SourceCompute = Source, Language = RenderingDevice.ShaderLanguage.Glsl };
        RDShaderSpirV spirv = _rd.ShaderCompileSpirVFromSource(source);
        string error = spirv.GetStageCompileError(RenderingDevice.ShaderStage.Compute);
        if (error.Length > 0)
        {
            GD.PushError("[wildstar_mount] " + GetType().Name + " shader: " + error);
            return;
        }

        _shader = _rd.ShaderCreateFromSpirV(spirv);
        if (!_shader.IsValid)
        {
            return;
        }

        _pipeline = _rd.ComputePipelineCreate(_shader);
        _sampler = _rd.SamplerCreate(new RDSamplerState
        {
            MinFilter = RenderingDevice.SamplerFilter.Linear,
            MagFilter = RenderingDevice.SamplerFilter.Linear,
            MipFilter = RenderingDevice.SamplerFilter.Nearest,
            RepeatU = RenderingDevice.SamplerRepeatMode.ClampToEdge,
            RepeatV = RenderingDevice.SamplerRepeatMode.ClampToEdge,
            RepeatW = RenderingDevice.SamplerRepeatMode.ClampToEdge,
        });
    }

    protected Rid Sampler => _sampler;

    protected RenderingDevice? Device => _rd;

    protected abstract RDUniform? ExtraUniform(RenderSceneBuffersRD buffers, uint view);

    protected abstract float[]? FrameParameters(float[] published, RenderData renderData, Vector2I size);

    public override void _RenderCallback(int effectCallbackType, RenderData renderData)
    {
        if (_rd is null || !_pipeline.IsValid || !Enabled)
        {
            return;
        }

        float[]? published = _params;
        if (published is null)
        {
            return;
        }

        if (renderData.GetRenderSceneBuffers() is not RenderSceneBuffersRD buffers)
        {
            return;
        }

        Vector2I size = buffers.GetInternalSize();
        if (size.X == 0 || size.Y == 0)
        {
            return;
        }

        float[]? frame = FrameParameters(published, renderData, size);
        if (frame is null)
        {
            return;
        }

        byte[] bytes = new byte[frame.Length * 4];
        Buffer.BlockCopy(frame, 0, bytes, 0, bytes.Length);
        if (!_paramsBuffer.IsValid || _paramsSize != bytes.Length)
        {
            if (_paramsBuffer.IsValid)
            {
                _rd.FreeRid(_paramsBuffer);
            }

            _paramsBuffer = _rd.StorageBufferCreate((uint)bytes.Length, bytes);
            _paramsSize = bytes.Length;
        }
        else
        {
            _rd.BufferUpdate(_paramsBuffer, 0, (uint)bytes.Length, bytes);
        }

        uint groupsX = (uint)((size.X - 1) / 8 + 1);
        uint groupsY = (uint)((size.Y - 1) / 8 + 1);
        uint viewCount = buffers.GetViewCount();
        for (uint view = 0; view < viewCount; view++)
        {
            Rid colour = buffers.GetColorLayer(view);
            var imageUniform = new RDUniform { UniformType = RenderingDevice.UniformType.Image, Binding = 0 };
            imageUniform.AddId(colour);
            RDUniform? extra = ExtraUniform(buffers, view);
            if (extra is null)
            {
                continue;
            }

            var paramsUniform = new RDUniform { UniformType = RenderingDevice.UniformType.StorageBuffer, Binding = 2 };
            paramsUniform.AddId(_paramsBuffer);
            Rid set = UniformSetCacheRD.GetCache(_shader, 0, new Array<RDUniform> { imageUniform, extra, paramsUniform });

            long list = _rd.ComputeListBegin();
            _rd.ComputeListBindComputePipeline(list, _pipeline);
            _rd.ComputeListBindUniformSet(list, set, 0);
            _rd.ComputeListDispatch(list, groupsX, groupsY, 1);
            _rd.ComputeListEnd();
        }
    }

    public override void _Notification(int what)
    {
        if (what == NotificationPredelete && _rd is not null)
        {
            if (_paramsBuffer.IsValid)
            {
                _rd.FreeRid(_paramsBuffer);
            }

            if (_sampler.IsValid)
            {
                _rd.FreeRid(_sampler);
            }

            if (_shader.IsValid)
            {
                _rd.FreeRid(_shader);
            }
        }
    }

    protected static void PutMatrix(float[] target, int offset, Projection projection)
    {
        Vector4[] columns = { projection.X, projection.Y, projection.Z, projection.W };
        for (int c = 0; c < 4; c++)
        {
            target[offset + 4 * c] = columns[c].X;
            target[offset + 4 * c + 1] = columns[c].Y;
            target[offset + 4 * c + 2] = columns[c].Z;
            target[offset + 4 * c + 3] = columns[c].W;
        }
    }

    protected static void PutBasis(float[] target, int offset, Basis basis)
    {
        Vector3[] columns = { basis.Column0, basis.Column1, basis.Column2 };
        for (int c = 0; c < 3; c++)
        {
            target[offset + 4 * c] = columns[c].X;
            target[offset + 4 * c + 1] = columns[c].Y;
            target[offset + 4 * c + 2] = columns[c].Z;
            target[offset + 4 * c + 3] = 0.0f;
        }

        target[offset + 12] = 0.0f;
        target[offset + 13] = 0.0f;
        target[offset + 14] = 0.0f;
        target[offset + 15] = 1.0f;
    }
}

[Tool]
[GlobalClass]
public partial class SkyFogEffect : SkyScreenEffect
{
    public const int PublishedLength = 4 + 36 + 64;

    [Export] public bool DebugDirection { get; set; }

    [Export] public bool FlipY { get; set; } = true;

    protected override string Source => SkyShaders.FogCompute;

    protected override RDUniform? ExtraUniform(RenderSceneBuffersRD buffers, uint view)
    {
        Rid depth = buffers.GetDepthLayer(view);
        if (!depth.IsValid)
        {
            return null;
        }

        var uniform = new RDUniform { UniformType = RenderingDevice.UniformType.SamplerWithTexture, Binding = 1 };
        uniform.AddId(Sampler);
        uniform.AddId(depth);
        return uniform;
    }

    protected override float[]? FrameParameters(float[] published, RenderData renderData, Vector2I size)
    {
        if (published.Length < PublishedLength || published[2] < 0.5f)
        {
            return null;
        }

        RenderSceneData sceneData = renderData.GetRenderSceneData();
        Projection projection = sceneData.GetCamProjection();
        Transform3D camera = sceneData.GetCamTransform();
        float start = published[0];
        float end = published[1];
        float mid = (start + end) * 0.5f;
        if (end <= start || mid <= 0.0f)
        {
            return null;
        }

        float mid2 = mid * mid;
        float denominator = 1.0f - MathF.Pow(2.0f, -(end * end) / mid2);
        if (denominator <= 0.000001f)
        {
            return null;
        }

        var frame = new float[16 + 16 + 4 + 4 + 4 + 36 + 64];
        PutMatrix(frame, 0, projection.Inverse());
        PutBasis(frame, 16, camera.Basis);
        frame[32] = size.X;
        frame[33] = size.Y;
        frame[34] = mid2;
        frame[35] = 1.0f / denominator;
        frame[36] = DebugDirection ? 1.0f : 0.0f;
        frame[37] = FlipY ? 1.0f : 0.0f;
        frame[38] = published[3];
        frame[39] = 0.0f;
        frame[40] = projection.GetZNear();
        frame[41] = projection.GetZFar();
        frame[42] = 0.0f;
        frame[43] = 0.0f;
        System.Array.Copy(published, 4, frame, 44, 36 + 64);
        return frame;
    }
}

[Tool]
[GlobalClass]
public partial class SkyGradeEffect : SkyScreenEffect
{
    public const int PublishedLength = 12;

    public const int LutSize = 16;

    private Rid _lut;
    private volatile byte[]? _pendingLut;
    private byte[]? _currentLut;

    protected override string Source => SkyShaders.GradeCompute;

    public void PublishLut(byte[]? rgba) => _pendingLut = rgba ?? System.Array.Empty<byte>();

    protected override RDUniform? ExtraUniform(RenderSceneBuffersRD buffers, uint view)
    {
        RenderingDevice? rd = Device;
        if (rd is null)
        {
            return null;
        }

        byte[]? pending = _pendingLut;
        if (pending is not null && !ReferenceEquals(pending, _currentLut))
        {
            _currentLut = pending;
            if (pending.Length == LutSize * LutSize * LutSize * 4)
            {
                if (!_lut.IsValid)
                {
                    var format = new RDTextureFormat
                    {
                        TextureType = RenderingDevice.TextureType.Type3D,
                        Format = RenderingDevice.DataFormat.R8G8B8A8Unorm,
                        Width = LutSize,
                        Height = LutSize,
                        Depth = LutSize,
                        Mipmaps = 1,
                        ArrayLayers = 1,
                        UsageBits = RenderingDevice.TextureUsageBits.SamplingBit |
                                    RenderingDevice.TextureUsageBits.CanUpdateBit |
                                    RenderingDevice.TextureUsageBits.CanCopyToBit,
                    };
                    _lut = rd.TextureCreate(format, new RDTextureView(), new Array<byte[]> { pending });
                }
                else
                {
                    rd.TextureUpdate(_lut, 0, pending);
                }
            }
        }

        if (!_lut.IsValid)
        {
            var format = new RDTextureFormat
            {
                TextureType = RenderingDevice.TextureType.Type3D,
                Format = RenderingDevice.DataFormat.R8G8B8A8Unorm,
                Width = LutSize,
                Height = LutSize,
                Depth = LutSize,
                Mipmaps = 1,
                ArrayLayers = 1,
                UsageBits = RenderingDevice.TextureUsageBits.SamplingBit |
                            RenderingDevice.TextureUsageBits.CanUpdateBit |
                            RenderingDevice.TextureUsageBits.CanCopyToBit,
            };
            _lut = rd.TextureCreate(format, new RDTextureView(), new Array<byte[]> { IdentityLut() });
        }

        var uniform = new RDUniform { UniformType = RenderingDevice.UniformType.SamplerWithTexture, Binding = 1 };
        uniform.AddId(Sampler);
        uniform.AddId(_lut);
        return uniform;
    }

    public static byte[] IdentityLut()
    {
        var data = new byte[LutSize * LutSize * LutSize * 4];
        int i = 0;
        for (int b = 0; b < LutSize; b++)
        {
            for (int g = 0; g < LutSize; g++)
            {
                for (int r = 0; r < LutSize; r++)
                {
                    data[i++] = (byte)(r * 255 / (LutSize - 1));
                    data[i++] = (byte)(g * 255 / (LutSize - 1));
                    data[i++] = (byte)(b * 255 / (LutSize - 1));
                    data[i++] = 255;
                }
            }
        }

        return data;
    }

    protected override float[]? FrameParameters(float[] published, RenderData renderData, Vector2I size)
    {
        if (published.Length < PublishedLength || published[0] < 0.5f)
        {
            return null;
        }

        var frame = new float[16];
        frame[0] = size.X;
        frame[1] = size.Y;
        frame[4] = published[4];
        frame[5] = published[5];
        frame[6] = published[6];
        frame[7] = published[7];
        frame[8] = published[8];
        frame[9] = published[9];
        frame[10] = published[10];
        frame[11] = published[11];
        frame[12] = published[1] > 0.5f && _lut.IsValid && _currentLut is { Length: > 0 } ? 1.0f : 0.0f;
        frame[13] = published[2];
        return frame;
    }

    public override void _Notification(int what)
    {
        base._Notification(what);
        if (what == NotificationPredelete && Device is RenderingDevice rd && _lut.IsValid)
        {
            rd.FreeRid(_lut);
        }
    }
}
