#if TOOLS
using System;
using System.Collections.Generic;
using Godot;
using WildStar.GameTable;

namespace WildStar.Editor;

[Tool]
public partial class TblViewer : Window
{
    private const int PageSize = 200;
    private const int MaxMatches = 100000;
    private const int MinColumnWidth = 90;

    private TblReader _table = null!;
    private Tree _tree = null!;
    private Label _status = null!;
    private LineEdit _filter = null!;
    private Button _prev = null!;
    private Button _next = null!;

    private readonly List<int> _rows = new();
    private int _page;

    public static TblViewer Open(TblReader table, string path)
    {
        var viewer = new TblViewer();
        viewer._table = table;
        viewer.Title = table.Name.Length > 0 ? table.Name : path;
        viewer.Build(path);

        EditorInterface.Singleton.GetBaseControl().AddChild(viewer);
        viewer.PopupCentered(new Vector2I(1200, 720));
        return viewer;
    }

    private void Build(string path)
    {
        CloseRequested += QueueFree;

        var root = new VBoxContainer { AnchorRight = 1, AnchorBottom = 1 };
        root.AddThemeConstantOverride("separation", 6);
        AddChild(root);

        var header = new Label
        {
            Text = $"{path}\n{_table.RecordCount:N0} records  ·  {_table.FieldCount} fields  " +
                   $"·  {_table.RecordSize} bytes/record  ·  {_table.LookupCount:N0} ids",
        };
        root.AddChild(header);

        var bar = new HBoxContainer();
        root.AddChild(bar);

        _filter = new LineEdit
        {
            PlaceholderText = "Filter rows…",
            SizeFlagsHorizontal = Control.SizeFlags.ExpandFill,
        };
        _filter.TextSubmitted += _ => ApplyFilter();
        bar.AddChild(_filter);

        var apply = new Button { Text = "Filter" };
        apply.Pressed += ApplyFilter;
        bar.AddChild(apply);

        _prev = new Button { Text = "◀" };
        _prev.Pressed += () => Turn(-1);
        bar.AddChild(_prev);

        _status = new Label { CustomMinimumSize = new Vector2(220, 0) };
        _status.HorizontalAlignment = HorizontalAlignment.Center;
        bar.AddChild(_status);

        _next = new Button { Text = "▶" };
        _next.Pressed += () => Turn(1);
        bar.AddChild(_next);

        _tree = new Tree
        {
            SizeFlagsVertical = Control.SizeFlags.ExpandFill,
            SizeFlagsHorizontal = Control.SizeFlags.ExpandFill,
            HideRoot = true,
            ColumnTitlesVisible = true,
            Columns = Math.Max(_table.FieldCount, 1),
            SelectMode = Tree.SelectModeEnum.Row,
        };

        for (int i = 0; i < _table.FieldCount; i++)
        {
            TblReader.FieldDesc f = _table.Fields[i];
            _tree.SetColumnTitle(i, f.Name);
            _tree.SetColumnCustomMinimumWidth(i, MinColumnWidth);
            _tree.SetColumnExpand(i, false);
            _tree.SetColumnClipContent(i, true);
        }

        root.AddChild(_tree);

        ResetRows();
    }

    private void ResetRows()
    {
        _rows.Clear();
        for (int i = 0; i < _table.RecordCount; i++)
        {
            _rows.Add(i);
        }

        _page = 0;
        Refresh();
    }

    private void ApplyFilter()
    {
        string needle = _filter.Text.Trim();
        if (needle.Length == 0)
        {
            ResetRows();
            return;
        }

        _rows.Clear();
        for (int i = 0; i < _table.RecordCount && _rows.Count < MaxMatches; i++)
        {
            for (int f = 0; f < _table.FieldCount; f++)
            {
                if (_table.GetText(i, f).Contains(needle, StringComparison.OrdinalIgnoreCase))
                {
                    _rows.Add(i);
                    break;
                }
            }
        }

        _page = 0;
        Refresh();
    }

    private void Turn(int delta)
    {
        int pages = PageCount();
        _page = Math.Clamp(_page + delta, 0, Math.Max(pages - 1, 0));
        Refresh();
    }

    private int PageCount() => Math.Max((_rows.Count + PageSize - 1) / PageSize, 1);

    private void Refresh()
    {
        _tree.Clear();
        TreeItem root = _tree.CreateItem();

        int start = _page * PageSize;
        int end = Math.Min(start + PageSize, _rows.Count);

        for (int r = start; r < end; r++)
        {
            int record = _rows[r];
            TreeItem item = _tree.CreateItem(root);

            for (int f = 0; f < _table.FieldCount; f++)
            {
                item.SetText(f, _table.GetText(record, f));
            }
        }

        int pages = PageCount();
        string shown = _rows.Count == 0
            ? "no rows"
            : $"{start + 1:N0}–{end:N0} of {_rows.Count:N0}";
        _status.Text = $"{shown}   (page {_page + 1}/{pages})";

        _prev.Disabled = _page <= 0;
        _next.Disabled = _page >= pages - 1;
    }
}
#endif
