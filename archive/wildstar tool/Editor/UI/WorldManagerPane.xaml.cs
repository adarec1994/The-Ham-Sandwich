using BCnEncoder.Decoder;
using BCnEncoder.Shared;
using MathUtils;
using OpenTK.Wpf;
using ProjectWS.Editor.Project;
using ProjectWS.Engine.Data;
using ProjectWS.Engine.Project;
using ProjectWS.Engine.Rendering;
using ProjectWS.Engine.World;
using SixLabors.ImageSharp.PixelFormats;
using SixLabors.ImageSharp;
using System;
using System.Collections.Generic;
using System.Collections.ObjectModel;
using System.Diagnostics;
using System.Drawing;
using System.IO;
using System.Runtime.InteropServices;
using System.Text.Json;
using System.Windows;
using System.Windows.Media;
using SixLabors.ImageSharp.Processing;
using System.Windows.Controls;
using System.Windows.Documents;

namespace ProjectWS.Editor
{
    /// <summary>
    /// Interaction logic for UserControl1.xaml
    /// </summary>
    public partial class WorldManagerPane : System.Windows.Controls.UserControl
    {
        public Editor editor;
        public MapRenderer mapRenderer;

        public ObservableCollection<string>? mapNames { get; set; }
        public List<uint>? mapIDs { get; set; }
        public ObservableCollection<string>? locationNames { get; set; }
        public List<uint>? locationIDs { get; set; }
        public uint selectedMapID;

        public WorldManagerPane(Editor editor, MapRenderer mapRenderer)
        {
            InitializeComponent();
            this.editor = editor;
            this.mapRenderer = mapRenderer;
            this.mapRenderer.onRightClick = OpenContextMenu;
            this.mapNames = new ObservableCollection<string>();
            this.mapIDs = new List<uint>();
            this.locationNames = new ObservableCollection<string>();
            this.locationIDs = new List<uint>();

            if (ProjectManager.project == null)
                return;

            if (ProjectManager.project.Maps == null)
                return;

            foreach (ProjectWS.Engine.Project.Project.Map map in ProjectManager.project!.Maps!)
            {
                this.mapNames.Add($"{map.worldRecord.ID}. {map.Name}");
                this.mapIDs.Add(map.worldRecord.ID);
            }

            this.mapComboBox.ItemsSource = this.mapNames;

            if (ProjectManager.project?.previousOpenMapID != 0)
            {
                this.mapComboBox.SelectedIndex = this.mapIDs.IndexOf(ProjectManager.project.previousOpenMapID);
            }

            this.mapRenderer.onCellHighlight += OnCellHighlight;
            this.mapRenderer.onZoomLevelChanged += OnZoomLevelChanged;
        }

        public void OnCellHighlight(Vector2i coords)
        {
            this.textBlock_HighlightedChunk.Text = $"Mouse Over: ({coords.X}, {coords.Y})";
        }

        public void OnZoomLevelChanged(float zoom)
        {
            this.textBlock_ZoomLevel.Text = $"Zoom: {zoom}";
        }

        public GLWpfControl GetOpenTKControl()
        {
            return this.GLWpfControl;
        }

        public System.Windows.Controls.Grid GetRendererGrid()
        {
            return this.RendererGrid;
        }

        private void button_AddChunk_Click(object sender, RoutedEventArgs e)
        {

        }

        private void button_RemoveChunk_Click(object sender, RoutedEventArgs e)
        {

        }

        private void button_AddMap_Click(object sender, RoutedEventArgs e)
        {
            editor.OpenCreateMapWindow();
        }

        private void button_RemoveMap_Click(object sender, RoutedEventArgs e)
        {
            editor.RemoveMap();
        }

        private void mapComboBox_SelectionChanged(object sender, System.Windows.Controls.SelectionChangedEventArgs e)
        {
            if (this.mapComboBox.SelectedIndex == -1) return;

            Debug.Log("Changed Map");

            SaveMapPosition();

            var mapID = this.mapIDs[this.mapComboBox.SelectedIndex];
            this.selectedMapID = mapID;
            var mapIndexInProject = -1;

            for (int i = 0; i < ProjectManager.project?.Maps?.Count; i++)
            {
                if (ProjectManager.project.Maps[i].worldRecord.ID == mapID)
                {
                    mapIndexInProject = i;
                    break;
                }
            }

            if (mapIndexInProject == -1) return;

            Engine.Project.Project.Map map = ProjectManager.project.Maps[mapIndexInProject];

            // Remember map for next session
            if (ProjectManager.project != null && this.mapComboBox != null && this.mapIDs != null)
            {
                if (this.mapComboBox.SelectedIndex == -1)
                    return;

                if (ProjectManager.project.previousOpenMapID != this.mapIDs[this.mapComboBox.SelectedIndex])
                {
                    ProjectManager.project.previousOpenMapID = (uint)this.mapIDs[this.mapComboBox.SelectedIndex];
                    ProjectManager.SaveProject();
                }
            }

            // Load chunk info
            ProjectWS.Editor.Project.MapChunkInfo? chunkInfo = null;
            if (File.Exists(map.mapChunkInfoPath))
            {
                string? jsonString = File.ReadAllText(map.mapChunkInfoPath);
                if (jsonString != null && jsonString != String.Empty)
                {
                    chunkInfo = JsonSerializer.Deserialize<ProjectWS.Editor.Project.MapChunkInfo>(jsonString);
                }
            }

            // Clear old map
            this.mapRenderer.ClearMap();

            // Refresh map visual
            if (chunkInfo != null)
            {
                this.mapRenderer.RefreshMapView(chunkInfo.chunks);
            }

            // Load minimaps
            if (chunkInfo != null)
            {
                this.mapRenderer.RefreshMinimaps(chunkInfo.minimaps, ProjectManager.projectFile, map.worldRecord.assetPath, map.Name);
            }

            // Load world
            this.editor.LoadWorld(map, chunkInfo, map.lastPosition);
            this.editor.engine.world.renderer.viewports[0].mainCamera.transform.SetRotation(map.lastOrientation);
        }

        private void SaveMapPosition()
        {
            this.editor.SaveMapPosition();
        }

        private static void GenerateMinimap(MapChunkInfo? chunkInfo, Engine.Project.Project.Map map, string savePath)
        {
            if (chunkInfo != null)
            {
                var mip = 3;
                var singleResolution = 512;
                for (int i = 0; i < mip; i++)
                {
                    singleResolution /= 2;
                }
                var fullResolution = singleResolution * 128;

                using (Image<Rgba32> outputImage = new Image<Rgba32>(fullResolution, fullResolution))
                {
                    for (int m = 0; m < chunkInfo.minimaps?.Count; m++)
                    {
                        var coord = chunkInfo.minimaps[m];

                        string x = coord.X.ToString("X2").ToLower();
                        string y = coord.Y.ToString("X2").ToLower();
                        string projectFolder = $"{Path.GetDirectoryName(ProjectManager.projectFile)}\\{Path.GetFileNameWithoutExtension(ProjectManager.projectFile)}";
                        string path = $"{projectFolder}\\{map.worldRecord.assetPath}\\{map.Name}.{y}{x}.tex";

                        FileFormats.Tex.File tex = new FileFormats.Tex.File(path);

                        using (var str = File.OpenRead(path))
                        {
                            tex.Read(str);

                            switch (tex.header.textureType)
                            {
                                case FileFormats.Tex.TextureType.DXT1:
                                    BcDecoder decoder = new BcDecoder();
                                    var decoded = decoder.DecodeRaw(tex.mipData[(tex.mipData.Count - 1) - mip], singleResolution, singleResolution, CompressionFormat.Bc1);
                                    var rawMap = MemoryMarshal.Cast<ColorRgba32, byte>(decoded).ToArray();
                                    var point = new SixLabors.ImageSharp.Point(singleResolution * coord.X, singleResolution * coord.Y);

                                    using (Image<Rgba32> img = SixLabors.ImageSharp.Image.LoadPixelData<Rgba32>(rawMap, singleResolution, singleResolution))
                                    {
                                        // take the 2 source images and draw them onto the image
                                        outputImage.Mutate(o => o
                                            .DrawImage(img, point, 1f) // draw the first one top left
                                        );
                                    }
                                    break;
                                default:
                                    Debug.Log($"NYI: Minimap combining of texture format : {tex.header.textureType}");
                                    break;
                            }
                        }
                    }

                    outputImage.Save(savePath);
                }
            }
        }

        private void button_DeselectAll_Click(object sender, RoutedEventArgs e)
        {
            if (this.mapRenderer != null)
                this.mapRenderer.DeselectAllCells();
        }

        private void button_CutChunk_Click(object sender, RoutedEventArgs e)
        {

        }

        private void button_CopyChunk_Click(object sender, RoutedEventArgs e)
        {

        }

        private void button_PasteChunk_Click(object sender, RoutedEventArgs e)
        {

        }

        private void ToggleGridOn(object sender, RoutedEventArgs e)
        {
            if (this.mapRenderer != null)
                this.mapRenderer.showGrid = true;
        }

        private void ToggleGridOff(object sender, RoutedEventArgs e)
        {
            if (this.mapRenderer != null)
                this.mapRenderer.showGrid = false;
        }

        private void button_ImportMap_Click(object sender, RoutedEventArgs e)
        {
            this.editor.OpenImportMapWindow();
        }

        private void button_EditMap_Click(object sender, RoutedEventArgs e)
        {
            this.editor.EditMap();
        }

        private void comboBox_location_SelectionChanged(object sender, System.Windows.Controls.SelectionChangedEventArgs e)
        {

        }

        private void button_Select_Checked(object sender, RoutedEventArgs e)
        {
            if (this.mapRenderer != null)
                this.mapRenderer.singleSelect = true;
        }

        private void button_Select_Unchecked(object sender, RoutedEventArgs e)
        {
            if (this.mapRenderer != null)
                this.mapRenderer.singleSelect = false;
        }

        private void button_MarqueeSelect_Checked(object sender, RoutedEventArgs e)
        {
            if (this.mapRenderer != null)
                this.mapRenderer.marqueeSelect = true;
        }

        private void button_MarqueeSelect_Unchecked(object sender, RoutedEventArgs e)
        {
            if (this.mapRenderer != null)
                this.mapRenderer.marqueeSelect = false;
        }

        private void button_TeleportToCellInSandbox_Click(object sender, RoutedEventArgs e)
        {
            if (this.mapRenderer != null)
            {
                Vector3 worldCoords = Utilities.ChunkToWorldCoords(this.mapRenderer.selectedCell);
                //this.editor.SandboxTeleport(worldCoords.X + 256, 0, worldCoords.Z + 256, this.selectedMapID);
                this.editor.EngineTeleport(worldCoords.X + 256, 0, worldCoords.Z + 256);
            }
        }

        private void button_LoadMap_Click(object sender, RoutedEventArgs e)
        {
            //this.editor.LoadWorld();
        }

        public void OpenContextMenu()
        {
            ContextMenu cm = this.FindResource("contextMenuRMB") as ContextMenu;
            cm.PlacementTarget = this.GLWpfControl;
            cm.Items.Clear();

            Vector2i selectedChunk = new Vector2i((int)this.mapRenderer.mapRMBMax.X, (int)this.mapRenderer.mapRMBMax.Y);
            Vector3 worldCoords = Utilities.ChunkToWorldCoords(this.mapRenderer.mapRMBMax);

            MenuItem itemChunkLocation = new MenuItem();
            var location = $"Chunk {selectedChunk}";
            itemChunkLocation.Header = location;
            itemChunkLocation.Click += (e, a) => { Clipboard.SetData(DataFormats.Text, location); };
            cm.Items.Add(itemChunkLocation);

            MenuItem itemWorldLocation = new MenuItem();
            var wlocation = $"World {worldCoords}";
            itemWorldLocation.Header = wlocation;
            itemWorldLocation.Click += (e, a) => { Clipboard.SetData(DataFormats.Text, wlocation); };
            cm.Items.Add(itemWorldLocation);

            cm.Items.Add(new Separator());

            MenuItem itemCopy = new MenuItem();
            itemCopy.Header = "Copy";
            itemCopy.Click += (e, a) => { this.editor.CopyChunk(selectedChunk); };
            cm.Items.Add(itemCopy);

            MenuItem itemPaste = new MenuItem();
            itemPaste.Header = "Paste";
            itemPaste.Click += (e, a) => { this.editor.PasteChunk(selectedChunk); };
            cm.Items.Add(itemPaste);

            MenuItem itemDelete = new MenuItem();
            itemDelete.Header = "Delete";
            itemDelete.Click += (e, a) => { this.editor.DeleteChunk(selectedChunk); };
            cm.Items.Add(itemDelete);

            cm.Items.Add(new Separator());

            MenuItem itemTeleportWorld = new MenuItem();
            itemTeleportWorld.Header = "Teleport World";
            itemTeleportWorld.Click += (e, a) => { 
                this.editor.EngineTeleport(worldCoords.X, this.editor.GetRoughHeightAtLocation(worldCoords) + 5, worldCoords.Z); };
            cm.Items.Add(itemTeleportWorld);

            MenuItem itemTeleportGame = new MenuItem();
            itemTeleportGame.Header = "Teleport Game";
            itemTeleportGame.Click += (e, a) => {
                worldCoords.Y = this.editor.GetRoughHeightAtLocation(worldCoords) + 5;
                this.editor.SandboxTeleport(worldCoords.X, worldCoords.Y, worldCoords.Z, this.selectedMapID);
            };
            cm.Items.Add(itemTeleportGame);

            cm.IsOpen = true;
        }
    }
}