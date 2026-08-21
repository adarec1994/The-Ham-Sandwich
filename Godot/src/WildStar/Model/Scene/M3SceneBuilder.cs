using Godot;
using WildStar.Archive;
using WildStar.GameTable;

namespace WildStar.Model;

public static class M3SceneBuilder
{
    private const string SkeletonName = "Skeleton";
    private static System.Func<WsFileSystem?>? _fileSystemProvider;

    public static void SetFileSystem(System.Func<WsFileSystem?>? provider)
    {
        _fileSystemProvider = provider;
        ModelSequenceLookup.Clear();
    }

    public static Node3D Build(M3File model, ArrayMesh mesh, string name, byte[] modelData) =>
        Build(model, mesh, name, modelData, System.Array.Empty<int>());

    public static Node3D Build(M3File model, ArrayMesh mesh, string name, byte[] modelData,
                               int[] surfaceGeosets)
        => Build(model, mesh, name, modelData, surfaceGeosets, System.Array.Empty<int>());

    public static Node3D Build(M3File model, ArrayMesh mesh, string name, byte[] modelData,
                               int[] surfaceGeosets, int[] surfaceMaterials,
                               string modelPath = "")
    {
        ApplyMaterials(model, mesh, surfaceMaterials);

        var surfaceKeys = new int[surfaceGeosets.Length];
        for (int i = 0; i < surfaceGeosets.Length; i++)
        {
            surfaceKeys[i] = model.GeosetKey(surfaceGeosets[i]);
        }

        string[] presets = System.Array.Empty<string>();
        var fs = _fileSystemProvider?.Invoke();
        if (fs != null && modelPath.Length > 0)
        {
            int[][] raw = ModelMeshLookup.FindOutfitPresets(fs, modelPath);
            presets = new string[raw.Length];
            for (int i = 0; i < raw.Length; i++)
            {
                presets[i] = string.Join(",", raw[i]);
            }
        }

        var root = new M3ModelRoot
        {
            Name = name,
            SurfaceGeosets = surfaceGeosets,
            SurfaceKeys = surfaceKeys,
            OutfitPresets = presets,
        };
        var instance = new MeshInstance3D { Name = "Mesh", Mesh = mesh };

        if (model.HasAabb)
        {
            var min = new Vector3(model.AabbMin[0], model.AabbMin[1], model.AabbMin[2]);
            var max = new Vector3(model.AabbMax[0], model.AabbMax[1], model.AabbMax[2]);
            instance.CustomAabb = new Aabb(min, max - min);
        }

        Skeleton3D? skeleton = BuildSkeleton(model);

        if (skeleton is null)
        {
            root.AddChild(instance);
            instance.Owner = root;
            root.MeshPath = new NodePath("Mesh");
            return root;
        }

        root.AddChild(skeleton);
        skeleton.Owner = root;
        skeleton.AddChild(instance);
        instance.Owner = root;
        instance.Skeleton = new NodePath("..");
        instance.Skin = skeleton.CreateSkinFromRestTransforms();

        AttachAnimator(root, skeleton, model, modelData);
        return root;
    }

    private static readonly System.Collections.Generic.HashSet<int> AlphaTestedTextureSlots = new();

    private static void ApplyMaterials(M3File model, ArrayMesh mesh, int[] surfaceMaterials)
    {
        int surfaces = System.Math.Min(mesh.GetSurfaceCount(), surfaceMaterials.Length);

        for (int i = 0; i < surfaces; i++)
        {
            int index = surfaceMaterials[i];
            if (index < 0 || index >= model.Materials.Length)
            {
                continue;
            }

            M3Material material = model.Materials[index];
            if (material.Layers.Length == 0)
            {
                continue;
            }

            int slot = material.Layers[0].TextureA;
            if (slot < 0 || slot >= model.Textures.Length)
            {
                continue;
            }

            ImageTexture? albedo = M3TextureCache.Get(model.Textures[slot].Path);
            if (albedo is null)
            {
                continue;
            }

            int normalSlot = material.Layers[0].TextureB;
            ImageTexture? normal = normalSlot >= 0 && normalSlot < model.Textures.Length
                ? M3TextureCache.Get(model.Textures[normalSlot].Path)
                : null;

            var standard = new StandardMaterial3D
            {
                AlbedoTexture = albedo,
                TextureFilter = BaseMaterial3D.TextureFilterEnum.LinearWithMipmapsAnisotropic,
                Transparency = AlphaTestedTextureSlots.Contains(model.Textures[slot].Slot)
                    ? BaseMaterial3D.TransparencyEnum.AlphaScissor
                    : BaseMaterial3D.TransparencyEnum.Disabled,
                AlphaScissorThreshold = 0.5f,
                CullMode = BaseMaterial3D.CullModeEnum.Disabled,
            };

            if (normal is not null)
            {
                standard.NormalEnabled = true;
                standard.NormalTexture = normal;
            }

            mesh.SurfaceSetMaterial(i, standard);
        }
    }

    public static Skeleton3D? BuildSkeleton(M3File model)
    {
        if (model.Bones.Length == 0)
        {
            return null;
        }

        var skeleton = new Skeleton3D { Name = SkeletonName };

        for (int i = 0; i < model.Bones.Length; i++)
        {
            skeleton.AddBone(BoneName(i));
        }

        for (int i = 0; i < model.Bones.Length; i++)
        {
            M3Bone bone = model.Bones[i];

            if (!bone.IsRoot && bone.Parent < model.Bones.Length && bone.Parent != i)
            {
                skeleton.SetBoneParent(i, bone.Parent);
            }

            skeleton.SetBoneRest(i, M3Matrix.LocalRest(model, i));
        }

        skeleton.ResetBonePoses();
        return skeleton;
    }

    public static string BoneName(int index) => "bone" + index;

    private static void AttachAnimator(Node3D root, Skeleton3D skeleton, M3File model,
                                       byte[] modelData)
    {
        if (model.Animations.Length == 0)
        {
            return;
        }

        bool animated = false;
        foreach (M3Bone bone in model.Bones)
        {
            if (bone.IsAnimated)
            {
                animated = true;
                break;
            }
        }

        if (!animated)
        {
            return;
        }

        var fs = _fileSystemProvider?.Invoke();
        M3BakeResult bake = M3AnimationBaker.Build(
            model, skeleton, SkeletonName,
            fs is not null ? ModelSequenceLookup.Load(fs) : null);
        if (bake.Player is not null)
        {
            root.AddChild(bake.Player);
            bake.Player.Owner = root;

            string note = bake.Skipped > 0
                ? $", {bake.Skipped} skipped (key budget {M3AnimationBaker.KeyBudget:N0} reached)"
                : string.Empty;
            GD.Print($"[wildstar_mount] {root.Name}: baked {bake.Baked} of " +
                     $"{model.Animations.Length} clips, {bake.Keys:N0} keys{note}");
        }

        var animator = new M3AnimatedSkeleton
        {
            Name = "Animator",
            ModelData = modelData,
            SkeletonPath = new NodePath("../" + SkeletonName),
        };

        root.AddChild(animator);
        animator.Owner = root;
    }
}
