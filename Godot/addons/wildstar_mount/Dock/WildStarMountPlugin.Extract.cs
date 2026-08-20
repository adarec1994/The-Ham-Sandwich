#if TOOLS
using System;
using System.IO;
using Godot;
using WildStar.Archive;
using WildStar.Audio.Bank;

namespace WildStar.Editor;

public partial class WildStarMountPlugin
{
    private void ExtractFile(WsFile file)
    {
        try
        {
            string destination = Write(file);
            GD.Print("[wildstar_mount] " + file.QualifiedPath + " -> " + destination);
        }
        catch (Exception exception)
        {
            GD.PushError("[wildstar_mount] " + file.QualifiedPath + ": " + exception.Message);
        }
    }

    private void ExtractBankSound(string qualified, WwiseBankSound sound)
    {
        try
        {
            int hash = qualified.LastIndexOf('#');
            string bankPath = hash > 0 ? qualified[..hash] : qualified;

            if (!WsFileSystem.TrySplitQualified(bankPath, out string archive, out string inner))
            {
                return;
            }

            string destination = Path.Combine(EnsureExtractRoot(), archive,
                Path.ChangeExtension(inner.Replace('/', Path.DirectorySeparatorChar), null)!,
                sound.Name);

            Directory.CreateDirectory(Path.GetDirectoryName(destination)!);
            File.WriteAllBytes(destination, sound.ReadAllBytes());

            GD.Print("[wildstar_mount] " + qualified + " -> " + destination);
        }
        catch (Exception exception)
        {
            GD.PushError("[wildstar_mount] " + qualified + ": " + exception.Message);
        }
    }

    private void ConfirmExtractDirectory(WsDirectory directory)
    {
        int count = 0;
        ulong bytes = 0;
        foreach (WsFile file in directory.EnumerateFilesRecursive())
        {
            count++;
            bytes += file.UncompressedSize;
        }

        if (count == 0)
        {
            GD.Print("[wildstar_mount] " + directory.QualifiedPath + " holds no files");
            return;
        }

        ConfirmationDialog dialog = EnsureConfirm();
        dialog.DialogText =
            "Extract " + directory.QualifiedPath + "\n\n" +
            count.ToString("N0") + " files, " + FormatSize(bytes) + " uncompressed\n" +
            "into " + ExtractRoot();
        dialog.OkButtonText = "Extract";
        _confirmed = () => ExtractDirectory(directory, count);
        dialog.ResetSize();
        dialog.PopupCentered();
    }

    private void ExtractDirectory(WsDirectory directory, int count)
    {
        int written = 0;
        int failed = 0;

        foreach (WsFile file in directory.EnumerateFilesRecursive())
        {
            try
            {
                Write(file);
                written++;
            }
            catch (Exception exception)
            {
                failed++;
                if (failed <= 10)
                {
                    GD.PushWarning("[wildstar_mount] " + file.QualifiedPath + ": " +
                        exception.Message);
                }
            }
        }

        GD.Print("[wildstar_mount] extracted " + written + " of " + count + " files from " +
            directory.QualifiedPath + " into " + ExtractRoot() +
            (failed != 0 ? " (" + failed + " failed)" : ""));
    }

    private static string Write(WsFile file)
    {
        string root = EnsureExtractRoot();
        string destination = Path.Combine(
            root, file.Archive.Name, file.Path.Replace('/', Path.DirectorySeparatorChar));

        Directory.CreateDirectory(Path.GetDirectoryName(destination)!);
        File.WriteAllBytes(destination, file.ReadAllBytes());
        return destination;
    }

    private static string ExtractRoot()
    {
        string configured = ProjectSettings
            .GetSetting(ExtractDirectorySetting, DefaultExtractDirectory).AsString().Trim();

        if (configured.Length == 0)
        {
            configured = DefaultExtractDirectory;
        }

        return Path.GetFullPath(ProjectSettings.GlobalizePath(configured));
    }

    private static string EnsureExtractRoot()
    {
        string root = ExtractRoot();
        Directory.CreateDirectory(root);

        string projectRoot = Path.GetFullPath(ProjectSettings.GlobalizePath("res://"));
        string marker = Path.Combine(root, ".gdignore");

        if (!root.StartsWith(projectRoot, StringComparison.OrdinalIgnoreCase) ||
            !ProjectSettings.GetSetting(IgnoreExtractedSetting, true).AsBool())
        {
            if (File.Exists(marker))
            {
                File.Delete(marker);
            }

            return root;
        }

        if (!File.Exists(marker))
        {
            File.WriteAllBytes(marker, Array.Empty<byte>());
        }

        return root;
    }

    private ConfirmationDialog EnsureConfirm()
    {
        if (_confirm is not null && GodotObject.IsInstanceValid(_confirm))
        {
            return _confirm;
        }

        _confirm = new ConfirmationDialog { Title = "Extract from archive" };
        _confirm.Confirmed += () =>
        {
            Action? action = _confirmed;
            _confirmed = null;
            action?.Invoke();
        };

        EditorInterface.Singleton.GetBaseControl().AddChild(_confirm);
        return _confirm;
    }
}
#endif
