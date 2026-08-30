using System;
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
                               string modelPath = "", int[]? surfaceSubmeshes = null)
    {
        M3TextureCache.PartHint = modelPath.Length > 0 ? modelPath : name;
        int[] hidden = ApplyMaterials(model, mesh, surfaceMaterials, surfaceSubmeshes, name);

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
            SurfaceHidden = hidden,
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
        instance.Skin = BuildSkin(model);

        AttachAnimator(root, skeleton, model, modelData);
        return root;
    }

    public static bool IsBlended(M3Material material) => material.IsBlended;

    private static BaseMaterial3D.BlendModeEnum BlendModeOf(uint blend) =>
        blend switch
        {
            M3Material.BlendAdditive => BaseMaterial3D.BlendModeEnum.Add,
            M3Material.BlendAlphaAdditive => BaseMaterial3D.BlendModeEnum.Add,
            M3Material.BlendModulate => BaseMaterial3D.BlendModeEnum.Mul,
            M3Material.BlendModulate2X => BaseMaterial3D.BlendModeEnum.Mul,
            M3Material.BlendSubtract => BaseMaterial3D.BlendModeEnum.Sub,
            M3Material.BlendAdditiveAlt => BaseMaterial3D.BlendModeEnum.Add,
            M3Material.BlendSoftAdditive => BaseMaterial3D.BlendModeEnum.Add,
            _ => BaseMaterial3D.BlendModeEnum.Mix,
        };

    private static bool ForcesInstanceAlphaOff(uint blend) =>
        blend is M3Material.BlendAdditive or M3Material.BlendAlphaAdditive
                 or M3Material.BlendModulate or M3Material.BlendModulate2X
                 or M3Material.BlendSubtract or M3Material.BlendAdditiveAlt
                 or M3Material.BlendSoftAdditive;

    private static int[] ApplyMaterials(M3File model, ArrayMesh mesh, int[] surfaceMaterials,
                                        int[]? surfaceSubmeshes, string name)
    {
        int surfaces = System.Math.Min(mesh.GetSurfaceCount(), surfaceMaterials.Length);
        var hidden = new int[mesh.GetSurfaceCount()];
        int unresolved = 0;
        int blendedCount = 0;
        int cutoutCount = 0;
        int depthOnlyCount = 0;
        int hiddenCount = 0;
        string firstUnresolved = string.Empty;
        var reported = new System.Collections.Generic.HashSet<int>();

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

            M3MaterialLayer layer = material.Layers[0];
            int slot = layer.TextureA;
            if (slot < 0 || slot >= model.Textures.Length)
            {
                continue;
            }

            M3Submesh? submesh = surfaceSubmeshes is not null && i < surfaceSubmeshes.Length &&
                                 surfaceSubmeshes[i] >= 0 &&
                                 surfaceSubmeshes[i] < model.Submeshes.Length
                ? model.Submeshes[surfaceSubmeshes[i]]
                : null;

            M3RenderGroupState group = submesh.HasValue &&
                                       submesh.Value.RenderGroup < model.RenderGroups.Length
                ? model.RenderGroupStateAtRest(submesh.Value.RenderGroup)
                : model.RenderGroupStateAtRest(-1);

            bool groupTransparent = submesh.HasValue &&
                                    submesh.Value.RenderGroup < model.RenderGroups.Length &&
                                    model.RenderGroups[submesh.Value.RenderGroup].ForcesTransparency;

            if (!material.IsDrawn || !group.Visible ||
                (submesh.HasValue && submesh.Value.HiddenInMainPass))
            {
                hidden[i] = 1;
                hiddenCount++;
            }

            M3TextureAlpha alphaMode = layer.OpacityFromColourAlpha
                ? (layer.OpacityInverted ? M3TextureAlpha.Invert : M3TextureAlpha.Keep)
                : M3TextureAlpha.Opaque;

            ImageTexture? albedo = M3TextureCache.Get(model.Textures[slot].Path, alphaMode);
            if (albedo is null)
            {
                unresolved++;
                if (firstUnresolved.Length == 0)
                {
                    firstUnresolved = model.Textures[slot].Path;
                }

                continue;
            }

            int normalSlot = layer.TextureB;
            ImageTexture? normal = normalSlot >= 0 && normalSlot < model.Textures.Length
                ? M3TextureCache.Get(model.Textures[normalSlot].Path)
                : null;

            bool blended = material.IsBlended || groupTransparent || !group.IsOpaque;
            bool cutout = !blended && material.IsAlphaTested;

            float opacity = layer.OpacityScale;
            float instanceAlpha = ForcesInstanceAlphaOff(material.Blend) ? 1.0f : group.Alpha;
            var tint = new Color(group.ColourMultiply[0], group.ColourMultiply[1],
                                 group.ColourMultiply[2], opacity * instanceAlpha);

            if (blended) blendedCount++;
            if (cutout) cutoutCount++;

            var standard = new StandardMaterial3D
            {
                AlbedoTexture = albedo,
                AlbedoColor = tint,
                TextureFilter = BaseMaterial3D.TextureFilterEnum.LinearWithMipmapsAnisotropic,
                AlphaScissorThreshold = M3Material.AlphaTestReference,
                Transparency = blended
                    ? BaseMaterial3D.TransparencyEnum.Alpha
                    : cutout
                        ? BaseMaterial3D.TransparencyEnum.AlphaScissor
                        : BaseMaterial3D.TransparencyEnum.Disabled,
                BlendMode = blended
                    ? BlendModeOf(material.Blend)
                    : BaseMaterial3D.BlendModeEnum.Mix,
                CullMode = material.IsTwoSided
                    ? BaseMaterial3D.CullModeEnum.Disabled
                    : BaseMaterial3D.CullModeEnum.Back,
            };

            if (!material.WritesDepth)
            {
                standard.DepthDrawMode = BaseMaterial3D.DepthDrawModeEnum.Disabled;
            }

            if (material.DepthTestAlways)
            {
                standard.NoDepthTest = true;
            }

            if (material.IsDepthOnly)
            {
                standard.Transparency = BaseMaterial3D.TransparencyEnum.Alpha;
                standard.BlendMode = BaseMaterial3D.BlendModeEnum.Mix;
                standard.AlbedoColor = new Color(1.0f, 1.0f, 1.0f, 0.0f);
                standard.DepthDrawMode = BaseMaterial3D.DepthDrawModeEnum.Always;
                depthOnlyCount++;
            }

            if (normal is not null)
            {
                standard.NormalEnabled = true;
                standard.NormalTexture = normal;
            }

            mesh.SurfaceSetMaterial(i, standard);

            if (!reported.Add(index))
            {
                continue;
            }

            string mode = material.IsDepthOnly ? "depth-only"
                        : blended ? "blend" : cutout ? "scissor" : "opaque";
            GD.Print($"[wildstar_mount]   mat {index}: type={material.Type} " +
                     $"blend={material.Blend} flags=0x{material.Flags:X} " +
                     $"opacitySrc={layer.OpacitySource} opacity={opacity:0.###} " +
                     $"groupAlpha={group.Alpha:0.###} -> {mode}  " +
                     model.Textures[slot].Path);
        }

        if (unresolved > 0 || blendedCount > 0 || cutoutCount > 0 || depthOnlyCount > 0 ||
            hiddenCount > 0)
        {
            string note = unresolved > 0
                ? $", {unresolved} surface(s) with no texture (first: {firstUnresolved})"
                : string.Empty;
            GD.Print($"[wildstar_mount] {name}: {surfaces} surfaces, " +
                     $"{blendedCount} blended, {cutoutCount} cutout, " +
                     $"{depthOnlyCount} depth-only, {hiddenCount} hidden{note}");
        }

        return hidden;
    }

    public static Skeleton3D? BuildSkeleton(M3File model)
    {
        if (model.Bones.Length == 0)
        {
            return null;
        }

        var skeleton = new Skeleton3D { Name = SkeletonName };
        var rest = new M3PoseRuntime(model);

        for (int i = 0; i < model.Bones.Length; i++)
        {
            skeleton.AddBone(BoneName(model, i));
        }

        for (int i = 0; i < model.Bones.Length; i++)
        {
            ReadOnlySpan<float> translation = rest.WorldTranslation(i);
            ReadOnlySpan<float> rotation = rest.WorldRotation(i);
            ReadOnlySpan<float> scale = rest.WorldScale(i);

            skeleton.SetBoneRest(i, M3Matrix.ToTransform(rest.World(i)));
            skeleton.SetBonePosePosition(
                i, new Vector3(translation[0], translation[1], translation[2]));
            skeleton.SetBonePoseRotation(
                i, new Quaternion(rotation[0], rotation[1], rotation[2], rotation[3]));
            skeleton.SetBonePoseScale(i, new Vector3(scale[0], scale[1], scale[2]));
        }

        return skeleton;
    }

    private static Skin BuildSkin(M3File model)
    {
        var skin = new Skin();
        skin.SetBindCount(model.Bones.Length);
        for (int i = 0; i < model.Bones.Length; i++)
        {
            skin.SetBindBone(i, i);
            skin.SetBindPose(i, M3Matrix.ToTransform(model.Bones[i].InverseBind));
        }

        return skin;
    }

    public static string BoneName(int index) => "bone" + index;

    public static string BoneName(M3File model, int index)
    {
        M3Bone bone = model.Bones[index];
        return bone.HasName ? $"bone{index}_{bone.NameHash:X8}" : BoneName(index);
    }

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
                ? $", {bake.Skipped} zero-length skipped"
                : string.Empty;
            GD.Print($"[wildstar_mount] {root.Name}: registered {bake.Baked} of " +
                     $"{model.Animations.Length} runtime-driven clips{note}");
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
