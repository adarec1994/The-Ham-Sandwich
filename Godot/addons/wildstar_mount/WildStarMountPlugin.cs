#if TOOLS
using System;
using System.Collections.Generic;
using System.IO;
using System.Threading.Tasks;
using Godot;
using WildStar.Archive;

namespace WildStar.Editor;

[Tool]
public partial class WildStarMountPlugin : EditorPlugin
{
    private const string GameDirectorySetting = "wildstar/paths/game_directory";
    private const string ExtractDirectorySetting = "wildstar/paths/extract_directory";
    private const string DefaultExtractDirectory = "res://extracted";
    private const string IgnoreExtractedSetting = "wildstar/paths/ignore_extracted";

    private const string DefaultSkySetting = "wildstar/view/default_sky";
    private const string DefaultSkyPath = "Sky\\TestReference.sky";

    private const string PlaceholderMeta = "__ws_placeholder__";
    private const string RootPrefix = "wsroot:";
    private const string DirectoryPrefix = "wsdir:";
    private const string BankPrefix = "wsbank:";

    private const string MapScenePrefix = "wsmap:";
    private const string BankSoundPrefix = "wsbanksound:";

    private const double WatchdogSeconds = 0.5;

    private static readonly Color ArchiveColor = new(0.45f, 0.72f, 1.0f);

    private readonly List<TreeItem> _roots = new();
    private readonly List<TreeItem> _pending = new();
    private readonly HashSet<string> _expanded = new(StringComparer.Ordinal);

    private WsFileSystem? _filesystem;
    private Tree? _tree;
    private Tree? _hookedTree;
    private GodotObject? _bridge;
    private PopupMenu? _menu;
    private ConfirmationDialog? _confirm;
    private Button? _picker;
    private EditorFileDialog? _picking;
    private WemImportPlugin? _wemImporter;
    private Action? _confirmed;
    private Task? _mountTask;
    private double _sinceCheck;
    private bool _reinjected;
    private bool _mounting;

    internal static WildStarMountPlugin? Instance { get; private set; }

    public override void _EnterTree()
    {
        Instance = this;
        EnsureSetting(GameDirectorySetting, "");
        EnsureSetting(ExtractDirectorySetting, DefaultExtractDirectory);
        EnsureSetting(IgnoreExtractedSetting, true);
        EnsureSetting(DefaultSkySetting, DefaultSkyPath);

        StartMount();

        _wemImporter = new WemImportPlugin();
        AddImportPlugin(_wemImporter);

        InstallModelLoader();

        EditorInterface.Singleton.GetResourceFilesystem().FilesystemChanged += OnFilesystemChanged;
        CallDeferred(nameof(EnsurePicker));
        SetProcess(true);
    }

    public override void _Process(double delta)
    {
        _sinceCheck += delta;
        if (_sinceCheck < WatchdogSeconds)
        {
            return;
        }

        _sinceCheck = 0;

        if (_filesystem is null || _filesystem.Archives.Count == 0)
        {
            return;
        }

        EnsureLoaders();

        if (_roots.Count == 0 || !GodotObject.IsInstanceValid(_roots[0]))
        {
            if (!_reinjected)
            {
                _reinjected = true;
                GD.Print("[wildstar_mount] dock rebuilt its tree; restoring the archive roots " +
                    "(this is expected and will not be logged again)");
            }

            Inject();
        }

        EnsurePreviewSky();
        KeepPickerOnTop();
    }

    public override void _ExitTree()
    {
        if (Instance == this)
        {
            Instance = null;
        }

        SetProcess(false);
        EditorInterface.Singleton.GetResourceFilesystem().FilesystemChanged -= OnFilesystemChanged;

        if (_wemImporter is not null)
        {
            RemoveImportPlugin(_wemImporter);
            _wemImporter = null;
        }

        RemoveModelLoader();

        StopPreview();
        Unhook();
        RemoveRoots();
        FreeControl(ref _menu);
        FreeControl(ref _confirm);
        FreeControl(ref _picker);
        FreeControl(ref _picking);
        FreeControl(ref _preview);

        _filesystem?.Dispose();
        _filesystem = null;
    }

    private void StartMount()
    {
        if (_mounting)
        {
            return;
        }

        _mounting = true;

        string configured = ProjectSettings.GetSetting(GameDirectorySetting, "").AsString().Trim();
        string repositoryRoot = Path.GetFullPath(
            Path.Combine(ProjectSettings.GlobalizePath("res://"), ".."));

        _mountTask = Task.Run(() =>
        {
            WsFileSystem? mounted = null;
            try
            {
                string directory = configured.Length != 0
                    ? ProjectSettings.GlobalizePath(configured)
                    : WsFileSystem.AutoDetect(repositoryRoot) ?? string.Empty;

                if (directory.Length == 0)
                {
                    GD.PushWarning("[wildstar_mount] no WildStar install found. Set " +
                        GameDirectorySetting + " in Project Settings.");
                }
                else
                {
                    mounted = WsFileSystem.Mount(directory);

                    GD.Print("[wildstar_mount] mounted " + mounted.Archives.Count +
                        " archive(s) from " + directory);
                    foreach (WsArchive archive in mounted.Archives)
                    {
                        GD.Print("[wildstar_mount]   " + archive.Name + ":// — " +
                            archive.BlockCount.ToString("N0") + " blocks");
                    }

                    foreach (string warning in mounted.Warnings)
                    {
                        GD.PushWarning("[wildstar_mount] " + warning);
                    }
                }
            }
            catch (Exception exception)
            {
                GD.PushWarning("[wildstar_mount] " + exception.Message);
            }

            _filesystem = mounted;
            _mounting = false;
            CallDeferred(nameof(Inject));
            CallDeferred(nameof(EnsurePicker));
        });
    }

    private void Remount()
    {
        if (_mounting)
        {
            GD.Print("[wildstar_mount] a mount is already running");
            return;
        }

        WsFileSystem? old = _filesystem;
        _filesystem = null;
        _expanded.Clear();
        _thumbnails.Clear();
        RemoveRoots();
        old?.Dispose();
        StartMount();
    }

    private void OnFilesystemChanged() => CallDeferred(nameof(Inject));
}
#endif
