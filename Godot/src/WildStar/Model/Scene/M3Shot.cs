using System;
using System.IO;
using Godot;
using WildStar.Model;

namespace WildStar.Tools;

public partial class M3Shot : Node3D
{
    private const int Width = 960;
    private const int Height = 720;

    private Camera3D _camera = null!;
    private int _clip;
    private int[] _hideKeys = System.Array.Empty<int>();
    private int _frames;

    public override async void _Ready()
    {
        var environment = new Godot.Environment
        {
            BackgroundMode = Godot.Environment.BGMode.Color,
            BackgroundColor = new Color(0.09f, 0.10f, 0.13f),
            AmbientLightSource = Godot.Environment.AmbientSource.Color,
            AmbientLightColor = new Color(0.6f, 0.62f, 0.7f),
            AmbientLightEnergy = 1.1f,
        };

        AddChild(new WorldEnvironment { Environment = environment });

        var key = new DirectionalLight3D { LightEnergy = 1.5f };
        key.RotationDegrees = new Vector3(-42.0f, -125.0f, 0.0f);
        AddChild(key);

        var fill = new DirectionalLight3D { LightEnergy = 0.5f };
        fill.RotationDegrees = new Vector3(-20.0f, 60.0f, 0.0f);
        AddChild(fill);

        _camera = new Camera3D();
        AddChild(_camera);

        var fs = WildStar.Archive.WsFileSystem.Mount("C:/Users/pwd12/OneDrive/Documents/WildStar");
        M3TextureCache.SetResolver(path =>
        {
            if (fs.TryGetFile(path, out WildStar.Archive.WsFile direct))
            {
                return direct.ReadAllBytes();
            }

            foreach (var arc in fs.Archives)
            {
                if (fs.TryGetFile(arc.Name + "://" + path, out WildStar.Archive.WsFile f))
                {
                    return f.ReadAllBytes();
                }
            }

            return null;
        });

        string[] args = OS.GetCmdlineUserArgs();
        string outDir = args.Length > 0 ? args[0] : "shots";
        Directory.CreateDirectory(outDir);

        for (int i = 1; i < args.Length; i++)
        {
            if (args[i].StartsWith("clip="))
            {
                _clip = int.Parse(args[i]["clip=".Length..]);
                continue;
            }

            if (args[i].StartsWith("hide="))
            {
                string spec = args[i]["hide=".Length..];
                _hideKeys = spec.Length == 0
                    ? System.Array.Empty<int>()
                    : System.Array.ConvertAll(spec.Split(','), int.Parse);
                continue;
            }

            if (args[i].StartsWith("frames="))
            {
                _frames = int.Parse(args[i]["frames=".Length..]);
                continue;
            }

            await Capture(args[i], outDir);
        }

        GetTree().Quit();
    }

    private async System.Threading.Tasks.Task Capture(string path, string outDir)
    {
        string name = Path.GetFileNameWithoutExtension(path);

        byte[] bytes;
        try
        {
            bytes = File.ReadAllBytes(path);
        }
        catch (Exception e)
        {
            GD.Print($"[shot] {name}: read failed: {e.Message}");
            return;
        }

        if (!M3File.TryParse(bytes, out M3File model, out string parseError))
        {
            GD.Print($"[shot] {name}: parse failed: {parseError}");
            return;
        }

        if (!M3MeshBuilder.TryBuild(model, out ArrayMesh mesh, out int[] surfaceGeosets,
                                    out int[] surfaceMaterials, out string meshError))
        {
            GD.Print($"[shot] {name}: mesh failed: {meshError}");
            return;
        }

        Node3D root = M3SceneBuilder.Build(model, mesh, name, bytes, surfaceGeosets,
                                           surfaceMaterials);

        if (root is M3ModelRoot modelRoot)
        {
            GD.Print($"[shot] {name}: variant keys = [{string.Join(",", modelRoot.VariantKeys())}]");
            if (_hideKeys.Length > 0)
            {
                var all = modelRoot.VariantKeys();
                var active = new System.Collections.Generic.List<int>();
                foreach (int k in all)
                    if (System.Array.IndexOf(_hideKeys, k) < 0)
                        active.Add(k);
                modelRoot.SetActiveVariantKeys(active.ToArray());
                GD.Print($"[shot] hiding keys [{string.Join(",", _hideKeys)}]");
            }
        }

        int textured = 0;
        for (int i = 0; i < mesh.GetSurfaceCount(); i++)
        {
            if (mesh.SurfaceGetMaterial(i) is not null) textured++;
        }
        GD.Print($"[shot]   textured surfaces: {textured}/{mesh.GetSurfaceCount()}");
        AddChild(root);

        Aabb bounds = mesh.GetAabb();
        var skeleton = root.GetNodeOrNull<Skeleton3D>("Skeleton");
        var animator = root.GetNodeOrNull<M3AnimatedSkeleton>("Animator");

        GD.Print($"[shot] {name}: bones={model.Bones.Length} anims={model.Animations.Length} " +
                 $"surfaces={mesh.GetSurfaceCount()} animator={(animator is not null)}");
        GD.Print($"[shot]   mesh aabb pos={bounds.Position} size={bounds.Size}");

        int clip = _clip;
        if (animator is not null && model.Animations.Length > 0)
        {
            animator.PlayIndex(clip);
            M3Animation chosen = model.Animations[clip];
            GD.Print($"[shot]   clip {clip}: seq={chosen.SequenceId} var={chosen.Variation} " +
                     $"len={chosen.Seconds:F2}s");
        }

        if (skeleton is not null)
        {
            ReportBoneSpread(skeleton);
        }

        if (animator is not null)
        {
            animator.SeekNormalized(0.4);
            await ToSignal(GetTree(), SceneTree.SignalName.ProcessFrame);
            GD.Print($"[shot]   pose round-trip error = {animator.PoseRoundTripError():E3}");
        }

        if (_frames > 0 && animator is not null)
        {
            for (int f = 0; f < _frames; f++)
            {
                double at = (double)f / _frames;
                await Shoot(root, bounds, $"{outDir}/{name}_f{f:D3}.png", skeleton, false,
                            animator, at);
            }

            root.QueueFree();
            await ToSignal(GetTree(), SceneTree.SignalName.ProcessFrame);
            return;
        }

        if (animator is not null && skeleton is not null)
        {
            animator.Playing = false;
            animator.SetProcess(false);
            skeleton.ResetBonePoses();
            await Shoot(root, bounds, $"{outDir}/{name}_rest.png", skeleton, false, null, 0.0);
            await Shoot(root, bounds, $"{outDir}/{name}_restrig.png", skeleton, true, null, 0.0);
            animator.SetProcess(true);
            animator.Playing = true;
        }

        await Shoot(root, bounds, $"{outDir}/{name}_model.png", skeleton, false, animator, 0.0);
        await Shoot(root, bounds, $"{outDir}/{name}_rig.png", skeleton, true, animator, 0.0);

        if (animator is not null && model.Animations.Length > 0)
        {
            await Shoot(root, bounds, $"{outDir}/{name}_anim_a.png", skeleton, false, animator, 0.15);
            await Shoot(root, bounds, $"{outDir}/{name}_anim_b.png", skeleton, false, animator, 0.45);
            await Shoot(root, bounds, $"{outDir}/{name}_anim_c.png", skeleton, false, animator, 0.75);
        }

        root.QueueFree();
        await ToSignal(GetTree(), SceneTree.SignalName.ProcessFrame);
    }

    private async System.Threading.Tasks.Task Shoot(Node3D root, Aabb bounds, string file,
                                                    Skeleton3D? skeleton, bool rig,
                                                    M3AnimatedSkeleton? animator, double t)
    {

        if (animator is not null)
        {
            animator.SeekNormalized(t);
        }

        FrameCamera(bounds);

        MeshInstance3D? overlay = null;
        var mesh = root.GetNodeOrNull<MeshInstance3D>("Skeleton/Mesh")
                   ?? root.GetNodeOrNull<MeshInstance3D>("Mesh");

        if (rig && skeleton is not null)
        {
            overlay = BuildRigOverlay(skeleton);
            root.AddChild(overlay);
            if (mesh is not null)
            {
                mesh.Visible = false;
            }
        }

        await ToSignal(GetTree(), SceneTree.SignalName.ProcessFrame);
        await ToSignal(RenderingServer.Singleton, RenderingServer.SignalName.FramePostDraw);

        Image image = GetViewport().GetTexture().GetImage();
        image.SavePng(file);
        GD.Print($"[shot]   wrote {file}");

        if (overlay is not null)
        {
            overlay.QueueFree();
        }

        if (mesh is not null)
        {
            mesh.Visible = true;
        }
    }

    private static void ReportBoneSpread(Skeleton3D skeleton)
    {
        int count = skeleton.GetBoneCount();
        var distance = new float[count];
        Vector3 centre = Vector3.Zero;

        for (int b = 0; b < count; b++)
        {
            centre += skeleton.GetBoneGlobalPose(b).Origin;
        }

        centre /= Mathf.Max(count, 1);

        for (int b = 0; b < count; b++)
        {
            distance[b] = skeleton.GetBoneGlobalPose(b).Origin.DistanceTo(centre);
        }

        Array.Sort(distance);

        GD.Print($"[shot]   bone distance from centroid: " +
                 $"median={distance[count / 2]:F2} p90={distance[(int)(count * 0.9f)]:F2} " +
                 $"max={distance[count - 1]:F2}");
    }

    private void FrameCamera(Aabb bounds)
    {
        Vector3 centre = bounds.GetCenter();
        float radius = Mathf.Max(bounds.Size.Length() * 0.5f, 0.001f);
        float distance = radius / Mathf.Sin(Mathf.DegToRad(_camera.Fov * 0.5f)) * 1.25f;

        var direction = new Vector3(0.62f, 0.34f, 0.71f).Normalized();

        _camera.Near = Mathf.Max(distance * 0.001f, 0.001f);
        _camera.Far = distance * 8.0f + radius * 4.0f;
        _camera.Position = centre + direction * distance;
        _camera.LookAt(centre, Vector3.Up);
    }

    private static MeshInstance3D BuildRigOverlay(Skeleton3D skeleton)
    {
        var immediate = new ImmediateMesh();
        var material = new StandardMaterial3D
        {
            ShadingMode = BaseMaterial3D.ShadingModeEnum.Unshaded,
            VertexColorUseAsAlbedo = true,
            NoDepthTest = true,
        };

        immediate.SurfaceBegin(Mesh.PrimitiveType.Lines, material);

        for (int i = 0; i < skeleton.GetBoneCount(); i++)
        {
            int parent = skeleton.GetBoneParent(i);
            if (parent < 0)
            {
                continue;
            }

            Vector3 a = skeleton.GetBoneGlobalPose(i).Origin;
            Vector3 b = skeleton.GetBoneGlobalPose(parent).Origin;

            immediate.SurfaceSetColor(new Color(0.2f, 1.0f, 0.5f));
            immediate.SurfaceAddVertex(a);
            immediate.SurfaceSetColor(new Color(0.0f, 0.6f, 1.0f));
            immediate.SurfaceAddVertex(b);
        }

        immediate.SurfaceEnd();

        return new MeshInstance3D { Mesh = immediate, Name = "RigOverlay" };
    }
}
