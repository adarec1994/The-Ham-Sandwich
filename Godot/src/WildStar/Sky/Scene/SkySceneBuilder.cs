using System;
using System.Collections.Generic;
using Godot;
using WildStar.Model;

namespace WildStar.Sky;

public static class SkySceneBuilder
{
    private static Func<string, byte[]?>? _resolver;

    public const string SkyShader = SkyShaders.SkyDome;

    public static void SetResolver(Func<string, byte[]?>? resolver)
    {
        _resolver = resolver;
        SkyRoot.SetLutResolver(resolver);
        if (resolver is null)
        {
            ClearModelCache();
        }
    }

    public static SkyRoot Build(SkyFile sky, byte[] bytes, string name)
    {
        var root = new SkyRoot { Name = name, SkyData = bytes, SourcePath = sky.SourcePath, RunClock = true };

        var shader = new Shader { Code = SkyShader };
        var material = new ShaderMaterial { Shader = shader };
        var environment = new Godot.Environment
        {
            BackgroundMode = Godot.Environment.BGMode.Sky,
            Sky = new Godot.Sky { SkyMaterial = material },
            AmbientLightSource = Godot.Environment.AmbientSource.Color,
            TonemapMode = Godot.Environment.ToneMapper.Linear,
            FogEnabled = false,
        };

        var compositor = new Compositor
        {
            CompositorEffects = new Godot.Collections.Array<CompositorEffect> { new SkyFogEffect(), new SkyGradeEffect() },
        };
        var world = new WorldEnvironment { Name = SkyRoot.EnvironmentNode, Environment = environment, Compositor = compositor };
        root.AddChild(world);
        world.Owner = root;

        var sun = new DirectionalLight3D { Name = SkyRoot.SunNode, ShadowEnabled = false };
        root.AddChild(sun);
        sun.Owner = root;

        var models = new Node3D { Name = SkyRoot.ModelsNode };
        root.AddChild(models);
        models.Owner = root;

        var used = new HashSet<string>(StringComparer.Ordinal);
        for (int i = 0; i < sky.Models.Length; i++)
        {
            SkyModel record = sky.Models[i];
            Node3D node = BuildModel(record.Path, UniqueName(used, LeafOf(record.Path)));
            node.SetMeta(SkyRoot.ModelMeta, i);
            node.SetMeta("sky_sort", record.SortOrder);
            node.SetMeta("sky_kind", record.Kind);
            node.SetMeta("sky_path", record.Path);
            ApplyDrawOrder(node, record.SortOrder);
            models.AddChild(node);
            Own(node, root);
        }

        string[] glare = { sky.GlareModelA, sky.GlareModelB };
        for (int g = 0; g < glare.Length; g++)
        {
            if (glare[g].Length == 0)
            {
                continue;
            }

            Node3D node = BuildModel(glare[g], UniqueName(used, "Glare" + g + "_" + LeafOf(glare[g])));
            node.SetMeta(SkyRoot.GlareMeta, g);
            node.SetMeta("sky_path", glare[g]);
            models.AddChild(node);
            Own(node, root);
        }

        var particulates = new Node3D { Name = SkyRoot.ParticulatesNode };
        root.AddChild(particulates);
        particulates.Owner = root;
        for (int i = 0; i < sky.Particulates.Length && i < SkyFile.ParticulateLimit; i++)
        {
            string path = sky.Particulates[i];
            if (path.Length == 0)
            {
                continue;
            }

            foreach (M3ParticleNode node in BuildParticulate(path, UniqueName(used, "Particulate" + i + "_" + LeafOf(path))))
            {
                particulates.AddChild(node);
                node.Owner = root;
            }
        }

        root.SetMeta("sky_flags", sky.Flags);
        root.SetMeta("sky_sun_model", (int)sky.SunModel);
        root.SetMeta("sky_blend_scalar", sky.BlendScalar);
        root.SetMeta("sky_environment_map", sky.EnvironmentMap);
        root.SetMeta("sky_colour_lut", sky.ColourLut);
        root.SetMeta("sky_particulates", sky.Particulates);
        root.Apply();
        return root;
    }

    private static readonly Dictionary<string, Node3D> ModelTemplates = new(StringComparer.OrdinalIgnoreCase);

    public static void ClearModelCache()
    {
        foreach (Node3D template in ModelTemplates.Values)
        {
            if (GodotObject.IsInstanceValid(template))
            {
                template.QueueFree();
            }
        }

        ModelTemplates.Clear();
        ParticulateModels.Clear();
    }

    private static Node3D BuildModel(string path, string name)
    {
        string key = path.Replace('\\', '/');
        if (!ModelTemplates.TryGetValue(key, out Node3D? template) || !GodotObject.IsInstanceValid(template))
        {
            template = BuildModelUncached(path, LeafOf(path));
            ModelTemplates[key] = template;
        }

        var instance = (Node3D)template.Duplicate();
        instance.Name = name;
        MakeMaterialsUnique(instance);
        return instance;
    }

    public static void MakeMaterialsUnique(Node node)
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

            for (int s = 0; s < instance.Mesh.GetSurfaceCount(); s++)
            {
                Material? source = instance.GetSurfaceOverrideMaterial(s) ?? instance.Mesh.SurfaceGetMaterial(s);
                if (source is not null)
                {
                    instance.SetSurfaceOverrideMaterial(s, (Material)source.Duplicate());
                }
            }
        }
    }

    private static readonly Dictionary<string, M3File?> ParticulateModels = new(StringComparer.OrdinalIgnoreCase);

    private static List<M3ParticleNode> BuildParticulate(string path, string name)
    {
        var nodes = new List<M3ParticleNode>();
        string key = path.Replace('\\', '/');
        if (!ParticulateModels.TryGetValue(key, out M3File? model))
        {
            model = null;
            byte[]? bytes = _resolver?.Invoke(key);
            if (bytes is null)
            {
                GD.PushWarning("[wildstar_mount] sky particulate not found: " + path);
            }
            else if (!M3File.TryParse(bytes, out M3File parsed, out string parseError))
            {
                GD.PushWarning("[wildstar_mount] " + path + ": " + parseError);
            }
            else
            {
                model = parsed;
            }

            ParticulateModels[key] = model;
        }

        if (model is null)
        {
            return nodes;
        }

        for (int e = 0; e < model.ParticleEmitters.Length; e++)
        {
            M3ParticleNode? node = M3ParticleNode.Create(model, e, key, model.ParticleEmitters.Length > 1 ? name + "_" + e : name);
            if (node is null)
            {
                continue;
            }

            node.FollowCamera = true;
            nodes.Add(node);
        }

        return nodes;
    }

    private static Node3D BuildModelUncached(string path, string name)
    {
        byte[]? bytes = path.Length > 0 ? _resolver?.Invoke(path.Replace('\\', '/')) : null;
        if (bytes is null)
        {
            GD.PushWarning("[wildstar_mount] sky model not found: " + path);
            return new Node3D { Name = name };
        }

        if (!M3File.TryParse(bytes, out M3File model, out string parseError))
        {
            GD.PushWarning("[wildstar_mount] " + path + ": " + parseError);
            return new Node3D { Name = name };
        }

        if (!M3MeshBuilder.TryBuild(model, out ArrayMesh mesh, out int[] geosets, out int[] materials,
                                    out int[] submeshes, out string buildError))
        {
            GD.PushWarning("[wildstar_mount] " + path + ": " + buildError);
            return new Node3D { Name = name };
        }

        mesh.ResourceName = name;
        Node3D node = M3SceneBuilder.Build(model, mesh, name, bytes, geosets, materials,
                                           path.Replace('\\', '/'), submeshes);
        MakeSkyboxMaterials(node);
        return node;
    }

    public static void MakeSkyboxMaterials(Node node)
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

            instance.CastShadow = GeometryInstance3D.ShadowCastingSetting.Off;
            instance.IgnoreOcclusionCulling = true;
            for (int s = 0; s < instance.Mesh.GetSurfaceCount(); s++)
            {
                if (instance.GetActiveMaterial(s) is StandardMaterial3D material)
                {
                    material.ShadingMode = BaseMaterial3D.ShadingModeEnum.Unshaded;
                    material.DisableFog = true;
                }
            }
        }
    }

    private static void ApplyDrawOrder(Node3D node, int sortOrder)
    {
        int priority = Mathf.Clamp(sortOrder / 8, 0, 127);
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

            for (int s = 0; s < instance.Mesh.GetSurfaceCount(); s++)
            {
                if (instance.GetActiveMaterial(s) is Material material)
                {
                    material.RenderPriority = priority;
                }
            }
        }
    }

    private static void Own(Node node, Node owner)
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

    private static string LeafOf(string path)
    {
        int slash = path.LastIndexOfAny(new[] { '/', '\\' });
        string leaf = slash >= 0 ? path[(slash + 1)..] : path;
        int dot = leaf.LastIndexOf('.');
        return dot > 0 ? leaf[..dot] : leaf;
    }

    private static string UniqueName(HashSet<string> used, string name)
    {
        string candidate = name.Length > 0 ? name : "Model";
        int suffix = 2;
        while (!used.Add(candidate))
        {
            candidate = name + "_" + suffix++;
        }

        return candidate;
    }
}
