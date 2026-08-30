using System;
using Godot;
using WildStar.Archive;
using WildStar.Sky;

namespace WildStar.Tools;

public partial class SkySmokeTest : SceneTree
{
    private WsFileSystem? _fs;
    private int _frames;
    private string _output = "sky.png";
    private SkyRoot? _sky;

    public override void _Initialize()
    {
        string[] args = OS.GetCmdlineUserArgs();
        string game = args.Length > 0 ? args[0] : @"C:\Users\pwd12\OneDrive\Documents\WildStar";
        string skyPath = args.Length > 1 ? args[1] : "Sky/LevianBay_Foggy_002.sky";
        _output = args.Length > 2 ? args[2] : "sky.png";
        float time = args.Length > 3 ? float.Parse(args[3]) : 43200.0f;
        float pitch = args.Length > 4 ? float.Parse(args[4]) : 25.0f;

        _fs = WsFileSystem.Mount(game);
        Func<string, byte[]?> resolver = Read;
        WildStar.Model.M3TextureCache.SetResolver(resolver);
        WildStar.Model.M3SceneBuilder.SetFileSystem(() => _fs);
        SkySceneBuilder.SetResolver(resolver);
        WildStar.Area.AreaTables.SetResolver(resolver);
        WildStar.Area.AreaTables.SetFileSystem(() => _fs);

        var world = new Node3D { Name = "World" };
        Root.AddChild(world);

        SkyRoot? sky = WildStar.Area.AreaSceneBuilder.BuildSky(skyPath);
        if (sky is null)
        {
            GD.Print("FAIL: could not build " + skyPath);
            Quit(1);
            return;
        }

        sky.RunClock = false;
        sky.TimeOfDay = time;
        world.AddChild(sky);
        _sky = sky;

        var camera = new Camera3D { Far = 60000.0f, Near = 1.0f, Fov = 70.0f, Current = true };
        world.AddChild(camera);
        camera.Position = Vector3.Zero;
        camera.RotationDegrees = new Vector3(pitch, 0.0f, 0.0f);

        GD.Print($"sky {skyPath} time={time} pitch={pitch}");
    }

    public override bool _Process(double delta)
    {
        _frames++;
        if (_frames == 75 && _sky is not null)
        {
            foreach (Node child in _sky.GetNode<Node3D>(SkyRoot.ModelsNode).GetChildren())
            {
                if (child is Node3D node)
                {
                    var mesh = FindMesh(node);
                    GD.Print($"model {node.Name} visible={node.Visible} transp={mesh?.Transparency ?? -1:0.00} " +
                             $"rot={node.RotationDegrees}");
                }
            }

            var models = _sky.GetNode<Node3D>(SkyRoot.ModelsNode);
            GD.Print($"models scale={models.Scale}");

            if (_sky.State is SkyState state)
            {
                GD.Print($"state sun has={state.HasSun} colour=({state.SunColour[0]:0.###},{state.SunColour[1]:0.###},{state.SunColour[2]:0.###}) " +
                         $"dir=({state.SunDirection[0]:0.###},{state.SunDirection[1]:0.###},{state.SunDirection[2]:0.###})");
            }

            if (_sky.GetNodeOrNull<DirectionalLight3D>(SkyRoot.SunNode) is DirectionalLight3D sun)
            {
                GD.Print($"sun light visible={sun.Visible} colour={sun.LightColor} energy={sun.LightEnergy:0.###}");
            }

            if (_sky.GetNodeOrNull<WorldEnvironment>(SkyRoot.EnvironmentNode)?.Environment is Godot.Environment env)
            {
                GD.Print($"ambient colour={env.AmbientLightColor} energy={env.AmbientLightEnergy:0.###}");
            }

            Image img = Root.GetViewport().GetTexture().GetImage();
            img.SavePng(_output);
            GD.Print("screenshot " + _output + " " + img.GetWidth() + "x" + img.GetHeight());
            Quit(0);
        }

        return false;
    }

    private static MeshInstance3D? FindMesh(Node node)
    {
        if (node is MeshInstance3D m)
        {
            return m;
        }

        foreach (Node child in node.GetChildren())
        {
            MeshInstance3D? found = FindMesh(child);
            if (found is not null)
            {
                return found;
            }
        }

        return null;
    }

    private byte[]? Read(string path)
    {
        if (_fs is null)
        {
            return null;
        }

        foreach (WsArchive archive in _fs.Archives)
        {
            if (_fs.TryGetFile(archive.Name + "://" + path.Replace('\\', '/'), out WsFile file))
            {
                try
                {
                    return file.ReadAllBytes();
                }
                catch (Exception)
                {
                    return null;
                }
            }
        }

        return null;
    }
}
