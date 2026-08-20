using System;
using System.Collections.Generic;
using Godot;

namespace WildStar.Model;

[Tool]
[GlobalClass]
public partial class M3ModelRoot : Node3D
{
    private static ShaderMaterial? _hiddenMaterial;

    [Export] public int[] SurfaceGeosets { get; set; } = Array.Empty<int>();
    [Export] public int[] SurfaceKeys { get; set; } = Array.Empty<int>();
    [Export] public NodePath MeshPath { get; set; } = new NodePath("Skeleton/Mesh");
    [Export] public int[] HiddenVariantKeys { get; set; } = Array.Empty<int>();
    public int SelectedVariant { get; set; }

    private const string DropdownProperty = "Variants/Active";
    private const string CheckboxPrefix = "Variants/key_";

    public override void _Ready()
    {
        NotifyPropertyListChanged();
        ApplyVisibility();
    }

    public override Godot.Collections.Array<Godot.Collections.Dictionary> _GetPropertyList()
    {
        var list = new Godot.Collections.Array<Godot.Collections.Dictionary>();
        int[] keys = VariantKeys();
        if (keys.Length == 0)
            return list;

        string hint = "All";
        foreach (int key in keys)
            hint += "," + key;

        list.Add(new Godot.Collections.Dictionary
        {
            { "name", DropdownProperty },
            { "type", (int)Variant.Type.Int },
            { "usage", (int)(PropertyUsageFlags.Editor | PropertyUsageFlags.Storage) },
            { "hint", (int)PropertyHint.Enum },
            { "hint_string", hint },
        });

        foreach (int key in keys)
        {
            list.Add(new Godot.Collections.Dictionary
            {
                { "name", CheckboxPrefix + key },
                { "type", (int)Variant.Type.Bool },
                { "usage", (int)PropertyUsageFlags.Editor },
            });
        }

        return list;
    }

    public override Variant _Get(StringName property)
    {
        string name = property.ToString();

        if (name == DropdownProperty)
        {
            if (SelectedVariant <= 0)
                return 0;
            int[] keys = VariantKeys();
            int idx = Array.IndexOf(keys, SelectedVariant);
            return idx < 0 ? 0 : idx + 1;
        }

        if (name.StartsWith(CheckboxPrefix, StringComparison.Ordinal) &&
            int.TryParse(name.AsSpan(CheckboxPrefix.Length), out int key))
        {
            return Array.IndexOf(HiddenVariantKeys, key) < 0;
        }

        return default;
    }

    public override bool _Set(StringName property, Variant value)
    {
        string name = property.ToString();

        if (name == DropdownProperty)
        {
            int idx = value.AsInt32();
            if (idx <= 0)
            {
                SelectedVariant = 0;
                HiddenVariantKeys = Array.Empty<int>();
            }
            else
            {
                int[] keys = VariantKeys();
                if (idx - 1 < keys.Length)
                    SelectedVariant = keys[idx - 1];
            }
            ApplyVisibility();
            NotifyPropertyListChanged();
            return true;
        }

        if (name.StartsWith(CheckboxPrefix, StringComparison.Ordinal) &&
            int.TryParse(name.AsSpan(CheckboxPrefix.Length), out int key))
        {
            SelectedVariant = 0;
            var hidden = new List<int>(HiddenVariantKeys);
            if (value.AsBool())
                hidden.Remove(key);
            else if (!hidden.Contains(key))
                hidden.Add(key);
            HiddenVariantKeys = hidden.ToArray();
            ApplyVisibility();
            return true;
        }

        return false;
    }

    private void ApplyVisibility()
    {
        var instance = GetNodeOrNull<MeshInstance3D>(MeshPath)
                       ?? GetNodeOrNull<MeshInstance3D>("Mesh");
        if (instance is null) return;

        int surfaces = Math.Min(SurfaceKeys.Length, instance.GetSurfaceOverrideMaterialCount());

        if (SelectedVariant > 0)
        {
            for (int i = 0; i < surfaces; i++)
            {
                int key = SurfaceKeys[i];
                bool visible = key == M3File.UngatedGeoset ||
                               (key > 0 && key == SelectedVariant);
                instance.SetSurfaceOverrideMaterial(i, visible ? null : HiddenMaterial());
            }
        }
        else
        {
            for (int i = 0; i < surfaces; i++)
            {
                int key = SurfaceKeys[i];
                bool visible = key <= 0 || Array.IndexOf(HiddenVariantKeys, key) < 0;
                instance.SetSurfaceOverrideMaterial(i, visible ? null : HiddenMaterial());
            }
        }
    }

    private static ShaderMaterial HiddenMaterial()
    {
        if (_hiddenMaterial is null)
        {
            var shader = new Shader
            {
                Code = "shader_type spatial;\nvoid vertex() { POSITION = vec4(2.0, 2.0, 2.0, 1.0); }\n",
            };
            _hiddenMaterial = new ShaderMaterial { Shader = shader };
        }
        return _hiddenMaterial;
    }

    public void SetActiveVariantKeys(params int[] activeKeys)
    {
        var instance = GetNodeOrNull<MeshInstance3D>(MeshPath)
                       ?? GetNodeOrNull<MeshInstance3D>("Mesh");
        if (instance is null) return;

        int surfaces = Math.Min(SurfaceKeys.Length, instance.GetSurfaceOverrideMaterialCount());
        for (int i = 0; i < surfaces; i++)
        {
            int key = SurfaceKeys[i];
            bool visible = key == M3File.UngatedGeoset ||
                           (key > 0 && Array.IndexOf(activeKeys, key) >= 0);
            instance.SetSurfaceOverrideMaterial(i, visible ? null : HiddenMaterial());
        }
    }

    public int[] VariantKeys()
    {
        var seen = new SortedSet<int>();
        foreach (int key in SurfaceKeys)
            if (key > 0)
                seen.Add(key);
        var keys = new int[seen.Count];
        seen.CopyTo(keys);
        return keys;
    }

    public int[] GeosetIds()
    {
        var seen = new SortedSet<int>();
        foreach (int id in SurfaceGeosets)
            seen.Add(id);
        var ids = new int[seen.Count];
        seen.CopyTo(ids);
        return ids;
    }
}
