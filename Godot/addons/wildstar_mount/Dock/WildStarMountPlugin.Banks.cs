#if TOOLS
using System;
using Godot;
using WildStar.Archive;
using WildStar.Audio.Bank;

namespace WildStar.Editor;

public partial class WildStarMountPlugin
{
    private string _bankPath = string.Empty;
    private WwiseBank? _bank;

    private static bool IsBank(string name) =>
        name.EndsWith(".bnk", StringComparison.OrdinalIgnoreCase);

    private void PopulateBank(TreeItem parent, string qualifiedPath)
    {
        Tree? tree = ResolveTree();
        if (tree is null || !TryGetBank(qualifiedPath, out WwiseBank bank))
        {
            return;
        }

        if (bank.Sounds.Count == 0)
        {
            TreeItem empty = tree.CreateItem(parent);
            empty.SetText(0, "(no embedded audio)");
            empty.SetSelectable(0, false);
            empty.SetMetadata(0, PlaceholderMeta);
            return;
        }

        Texture2D icon = IconFor(".wem") ?? GetFileIcon();

        foreach (WwiseBankSound sound in bank.Sounds)
        {
            TreeItem item = tree.CreateItem(parent);
            item.SetText(0, sound.Name);
            item.SetMetadata(0, BankSoundPrefix + qualifiedPath + "#" + sound.Id);
            item.SetIcon(0, icon);
            item.SetTooltipText(0, qualifiedPath + "#" + sound.Id + "\n" +
                FormatSize((ulong)sound.Length) +
                (sound.IsPrefetch ? " prefetch header, streamed from " + sound.Name : " embedded"));

            if (sound.IsPrefetch)
            {
                item.SetIconModulate(0, ArchiveColor);
            }
        }
    }

    private bool TryGetBankSoundBytes(string meta, out byte[] bytes)
    {
        bytes = Array.Empty<byte>();

        if (!TryGetBankSound(meta, out WwiseBankSound sound))
        {
            return false;
        }

        if (sound.IsPrefetch && TryGetStreamedSound(meta, sound, out byte[] streamed))
        {
            bytes = streamed;
            return true;
        }

        bytes = sound.ReadAllBytes();
        return true;
    }

    private bool TryGetStreamedSound(string meta, WwiseBankSound sound, out byte[] bytes)
    {
        bytes = Array.Empty<byte>();

        int hash = meta.LastIndexOf('#');
        if (hash <= 0 || _filesystem is null)
        {
            return false;
        }

        string bankPath = meta[..hash];
        int slash = bankPath.LastIndexOf('/');
        if (slash <= 0)
        {
            return false;
        }

        if (!_filesystem.TryGetDirectory(bankPath[..slash], out WsDirectory directory) ||
            !directory.TryGetFile(sound.Name, out WsFile file))
        {
            return false;
        }

        try
        {
            bytes = file.ReadAllBytes();
            return true;
        }
        catch (Exception exception)
        {
            GD.PushWarning("[wildstar_mount] " + file.QualifiedPath + ": " + exception.Message);
            return false;
        }
    }

    private bool TryGetBankSound(string meta, out WwiseBankSound sound)
    {
        sound = null!;

        int hash = meta.LastIndexOf('#');
        if (hash <= 0 || !uint.TryParse(meta[(hash + 1)..], out uint id))
        {
            return false;
        }

        return TryGetBank(meta[..hash], out WwiseBank bank) && bank.TryGetSound(id, out sound);
    }

    private bool TryGetBank(string qualifiedPath, out WwiseBank bank)
    {
        if (_bank is not null && _bankPath == qualifiedPath)
        {
            bank = _bank;
            return true;
        }

        bank = null!;

        if (_filesystem is null || !_filesystem.TryGetFile(qualifiedPath, out WsFile file))
        {
            return false;
        }

        try
        {
            if (!WwiseBank.TryParse(file.ReadAllBytes(), out bank, out string error))
            {
                GD.PushError("[wildstar_mount] " + qualifiedPath + ": " + error);
                return false;
            }
        }
        catch (Exception exception)
        {
            GD.PushError("[wildstar_mount] " + qualifiedPath + ": " + exception.Message);
            return false;
        }

        _bank = bank;
        _bankPath = qualifiedPath;
        return true;
    }
}
#endif
