#if TOOLS
using System;
using System.Collections.Generic;
using Godot;
using WildStar.GameTable;

namespace WildStar.Editor;

[Tool]
public partial class TblViewer : Window
{
    private const int MaxMatches = 200000;
    private const int SampleRows = 300;
    private const int MinColumnWidth = 54;
    private const int MaxColumnWidth = 420;
    private const int ColumnPadding = 22;

    private TblReader _table = null!;
    private Tree _tree = null!;
    private Tree _detail = null!;
    private Label _status = null!;
    private LineEdit _filter = null!;
    private LineEdit _goto = null!;
    private LineEdit _pageBox = null!;
    private OptionButton _scope = null!;
    private OptionButton _pageSize = null!;
    private Button _prev = null!;
    private Button _next = null!;

    private readonly List<int> _rows = new();
    private int _page;
    private int _perPage = 200;
    private int _sortColumn = -1;
    private bool _sortDescending;

    public static TblViewer Open(TblReader table, string path)
    {
        var viewer = new TblViewer();
        viewer._table = table;
        viewer.Title = table.Name.Length > 0 ? table.Name : path;
        viewer.Build(path);

        EditorInterface.Singleton.GetBaseControl().AddChild(viewer);
        viewer.PopupCentered(new Vector2I(1380, 800));
        return viewer;
    }

    private void Build(string path)
    {
        CloseRequested += QueueFree;

        var root = new VBoxContainer { AnchorRight = 1, AnchorBottom = 1 };
        root.AddThemeConstantOverride("separation", 6);
        AddChild(root);

        root.AddChild(new Label
        {
            Text = $"{path}    ·    {_table.RecordCount:N0} records  ·  {_table.FieldCount} fields" +
                   $"  ·  {_table.RecordSize} bytes/record  ·  {_table.LookupCount:N0} ids",
        });

        BuildFilterBar(root);
        BuildPageBar(root);

        var split = new HSplitContainer
        {
            SizeFlagsVertical = Control.SizeFlags.ExpandFill,
            SizeFlagsHorizontal = Control.SizeFlags.ExpandFill,
            SplitOffsets = new[] { 980 },
        };
        root.AddChild(split);

        _tree = new Tree
        {
            SizeFlagsVertical = Control.SizeFlags.ExpandFill,
            SizeFlagsHorizontal = Control.SizeFlags.ExpandFill,
            HideRoot = true,
            ColumnTitlesVisible = true,
            Columns = Math.Max(_table.FieldCount, 1),
            SelectMode = Tree.SelectModeEnum.Row,
        };
        _tree.ColumnTitleClicked += OnTitleClicked;
        _tree.ItemSelected += ShowDetail;
        split.AddChild(_tree);

        _detail = new Tree
        {
            SizeFlagsVertical = Control.SizeFlags.ExpandFill,
            CustomMinimumSize = new Vector2(300, 0),
            HideRoot = true,
            ColumnTitlesVisible = true,
            Columns = 2,
            SelectMode = Tree.SelectModeEnum.Row,
        };
        _detail.SetColumnTitle(0, "Field");
        _detail.SetColumnTitle(1, "Value");
        _detail.SetColumnCustomMinimumWidth(0, 150);
        _detail.SetColumnExpand(0, false);
        split.AddChild(_detail);

        SetupColumns();
        ResetRows();
    }

    private void BuildFilterBar(VBoxContainer root)
    {
        var bar = new HBoxContainer();
        root.AddChild(bar);

        _filter = new LineEdit
        {
            PlaceholderText = "Filter rows…  (press Enter)",
            SizeFlagsHorizontal = Control.SizeFlags.ExpandFill,
            ClearButtonEnabled = true,
        };
        _filter.TextSubmitted += _ => ApplyFilter();
        bar.AddChild(_filter);

        bar.AddChild(new Label { Text = "in" });

        _scope = new OptionButton();
        _scope.AddItem("all columns", 0);
        for (int i = 0; i < _table.FieldCount; i++)
        {
            _scope.AddItem(_table.Fields[i].Name, i + 1);
        }
        bar.AddChild(_scope);

        var apply = new Button { Text = "Filter" };
        apply.Pressed += ApplyFilter;
        bar.AddChild(apply);

        var clear = new Button { Text = "Reset" };
        clear.Pressed += () =>
        {
            _filter.Text = string.Empty;
            _sortColumn = -1;
            ResetRows();
        };
        bar.AddChild(clear);
    }

    private void BuildPageBar(VBoxContainer root)
    {
        var bar = new HBoxContainer();
        root.AddChild(bar);

        _prev = new Button { Text = "◀" };
        _prev.Pressed += () => Turn(-1);
        bar.AddChild(_prev);

        _pageBox = new LineEdit
        {
            CustomMinimumSize = new Vector2(70, 0),
            Alignment = HorizontalAlignment.Center,
        };
        _pageBox.TextSubmitted += text =>
        {
            if (int.TryParse(text, out int page))
            {
                _page = Math.Clamp(page - 1, 0, PageCount() - 1);
                Refresh();
            }
        };
        bar.AddChild(_pageBox);

        _next = new Button { Text = "▶" };
        _next.Pressed += () => Turn(1);
        bar.AddChild(_next);

        _status = new Label { SizeFlagsHorizontal = Control.SizeFlags.ExpandFill };
        bar.AddChild(_status);

        bar.AddChild(new Label { Text = "rows/page" });
        _pageSize = new OptionButton();
        foreach (int n in new[] { 100, 200, 500, 1000 })
        {
            _pageSize.AddItem(n.ToString(), n);
        }
        _pageSize.Selected = 1;
        _pageSize.ItemSelected += index =>
        {
            _perPage = _pageSize.GetItemId((int)index);
            _page = 0;
            Refresh();
        };
        bar.AddChild(_pageSize);

        if (_table.LookupCount > 0)
        {
            bar.AddChild(new Label { Text = "  id" });
            _goto = new LineEdit
            {
                CustomMinimumSize = new Vector2(90, 0),
                PlaceholderText = "go to",
            };
            _goto.TextSubmitted += GoToId;
            bar.AddChild(_goto);
        }
        else
        {
            _goto = new LineEdit();
        }
    }

    private void SetupColumns()
    {
        Font? font = _tree.HasThemeFont("font") ? _tree.GetThemeFont("font") : null;
        int fontSize = _tree.HasThemeFontSize("font_size") ? _tree.GetThemeFontSize("font_size") : 14;

        int sample = Math.Min(_table.RecordCount, SampleRows);

        for (int f = 0; f < _table.FieldCount; f++)
        {
            string widest = _table.Fields[f].Name;
            for (int r = 0; r < sample; r++)
            {
                string text = _table.GetText(r, f);
                if (text.Length > widest.Length)
                {
                    widest = text;
                }
            }

            int width = font is null
                ? widest.Length * 8
                : (int)font.GetStringSize(widest, HorizontalAlignment.Left, -1, fontSize).X;

            _tree.SetColumnTitle(f, _table.Fields[f].Name);
            _tree.SetColumnCustomMinimumWidth(
                f, Math.Clamp(width + ColumnPadding, MinColumnWidth, MaxColumnWidth));
            _tree.SetColumnExpand(f, false);
            _tree.SetColumnClipContent(f, true);
        }
    }

    private void OnTitleClicked(long column, long button)
    {
        if (button != (long)MouseButton.Left)
        {
            return;
        }

        int index = (int)column;
        _sortDescending = _sortColumn == index && !_sortDescending;
        _sortColumn = index;
        Sort();
        _page = 0;
        Refresh();
    }

    private void Sort()
    {
        if (_sortColumn < 0 || _sortColumn >= _table.FieldCount)
        {
            return;
        }

        int field = _sortColumn;
        TblReader.FieldType type = _table.Fields[field].Type;
        int n = _rows.Count;

        if (type == TblReader.FieldType.String)
        {
            var keys = new string[n];
            for (int i = 0; i < n; i++)
            {
                keys[i] = _table.GetString(_rows[i], field);
            }

            int[] order = Order(n);
            Array.Sort(keys, order, StringComparer.OrdinalIgnoreCase);
            Reorder(order);
        }
        else
        {
            var keys = new double[n];
            for (int i = 0; i < n; i++)
            {
                keys[i] = type switch
                {
                    TblReader.FieldType.Single => _table.GetSingle(_rows[i], field),
                    TblReader.FieldType.ULong => _table.GetULong(_rows[i], field),
                    _ => _table.GetUInt(_rows[i], field),
                };
            }

            int[] order = Order(n);
            Array.Sort(keys, order);
            Reorder(order);
        }

        if (_sortDescending)
        {
            _rows.Reverse();
        }

        for (int f = 0; f < _table.FieldCount; f++)
        {
            string name = _table.Fields[f].Name;
            _tree.SetColumnTitle(f, f == field ? (_sortDescending ? name + "  ▼" : name + "  ▲") : name);
        }
    }

    private int[] Order(int n)
    {
        var order = new int[n];
        for (int i = 0; i < n; i++)
        {
            order[i] = _rows[i];
        }
        return order;
    }

    private void Reorder(int[] order)
    {
        _rows.Clear();
        _rows.AddRange(order);
    }

    private void ResetRows()
    {
        _rows.Clear();
        for (int i = 0; i < _table.RecordCount; i++)
        {
            _rows.Add(i);
        }

        _sortColumn = -1;
        for (int f = 0; f < _table.FieldCount; f++)
        {
            _tree.SetColumnTitle(f, _table.Fields[f].Name);
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

        int only = _scope.GetItemId(_scope.Selected) - 1;

        _rows.Clear();
        for (int i = 0; i < _table.RecordCount && _rows.Count < MaxMatches; i++)
        {
            if (only >= 0)
            {
                if (_table.GetText(i, only).Contains(needle, StringComparison.OrdinalIgnoreCase))
                {
                    _rows.Add(i);
                }

                continue;
            }

            for (int f = 0; f < _table.FieldCount; f++)
            {
                if (_table.GetText(i, f).Contains(needle, StringComparison.OrdinalIgnoreCase))
                {
                    _rows.Add(i);
                    break;
                }
            }
        }

        if (_sortColumn >= 0)
        {
            Sort();
        }

        _page = 0;
        Refresh();
    }

    private void GoToId(string text)
    {
        if (!uint.TryParse(text.Trim(), out uint id))
        {
            return;
        }

        int record = _table.RecordIndexForId(id);
        if (record < 0)
        {
            _status.Text = $"id {id} is not in the lookup table";
            return;
        }

        int at = _rows.IndexOf(record);
        if (at < 0)
        {
            _filter.Text = string.Empty;
            ResetRows();
            at = _rows.IndexOf(record);
            if (at < 0)
            {
                return;
            }
        }

        _page = at / _perPage;
        Refresh();
        SelectRow(at - _page * _perPage);
    }

    private void SelectRow(int indexOnPage)
    {
        TreeItem? root = _tree.GetRoot();
        TreeItem? item = root?.GetChild(indexOnPage);
        if (item is null)
        {
            return;
        }

        item.Select(0);
        _tree.ScrollToItem(item, true);
        ShowDetail();
    }

    private void Turn(int delta)
    {
        _page = Math.Clamp(_page + delta, 0, Math.Max(PageCount() - 1, 0));
        Refresh();
    }

    private int PageCount() => Math.Max((_rows.Count + _perPage - 1) / _perPage, 1);

    private void Refresh()
    {
        _tree.Clear();
        TreeItem root = _tree.CreateItem();

        int start = _page * _perPage;
        int end = Math.Min(start + _perPage, _rows.Count);

        for (int r = start; r < end; r++)
        {
            int record = _rows[r];
            TreeItem item = _tree.CreateItem(root);
            item.SetMetadata(0, record);

            for (int f = 0; f < _table.FieldCount; f++)
            {
                string text = _table.GetText(record, f);
                item.SetText(f, text);
                item.SetTooltipText(f, _table.Fields[f].Name + ":  " + text);

                if (_table.Fields[f].Type != TblReader.FieldType.String)
                {
                    item.SetTextAlignment(f, HorizontalAlignment.Right);
                }
            }
        }

        int pages = PageCount();
        _status.Text = _rows.Count == 0
            ? "no matching rows"
            : $"showing {start + 1:N0}–{end:N0} of {_rows.Count:N0}";
        _pageBox.Text = (_page + 1).ToString();
        _pageBox.TooltipText = $"page {_page + 1} of {pages}";

        _prev.Disabled = _page <= 0;
        _next.Disabled = _page >= pages - 1;

        _detail.Clear();
    }

    private void ShowDetail()
    {
        _detail.Clear();
        TreeItem? selected = _tree.GetSelected();
        if (selected is null)
        {
            return;
        }

        int record = selected.GetMetadata(0).AsInt32();
        TreeItem root = _detail.CreateItem();

        for (int f = 0; f < _table.FieldCount; f++)
        {
            TblReader.FieldDesc field = _table.Fields[f];
            TreeItem item = _detail.CreateItem(root);
            item.SetText(0, field.Name);
            item.SetTooltipText(0, $"{field.Type} @ record byte {field.RecordOffset}");
            item.SetText(1, _table.GetText(record, f));
            item.SetTooltipText(1, _table.GetText(record, f));
        }
    }
}
#endif
