using ProjectWS.Engine.Project;
using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Data;
using System.Windows.Documents;
using System.Windows.Input;
using System.Windows.Media;
using System.Windows.Media.Imaging;
using System.Windows.Shapes;
using static ProjectWS.Engine.Project.Project;

namespace ProjectWS.Editor.UI
{
    /// <summary>
    /// Interaction logic for MapEditorWindow.xaml
    /// </summary>
    public partial class MapEditorWindow : Window
    {
        public Editor editor { get; }
        public bool newMap;

        public MapEditorWindow(Editor editor, bool newMap)
        {
            InitializeComponent();
            this.editor = editor;
            this.newMap = newMap;
        }

        internal void FillInputs(Map map)
        {
            if (map.worldRecord == null) return;

            this.textBlock_MapID.Text = map.worldRecord.ID.ToString();
            this.textBox_MapName.Text = map.Name;

            this.checkBox_Flag0x1.IsChecked = (map.worldRecord.flags & 0x1) != 0;
            this.checkBox_Flag0x2.IsChecked = (map.worldRecord.flags & 0x2) != 0;
            this.checkBox_Flag0x4.IsChecked = (map.worldRecord.flags & 0x4) != 0;
            this.checkBox_Flag0x8.IsChecked = (map.worldRecord.flags & 0x8) != 0;
            this.checkBox_Flag0x10.IsChecked = (map.worldRecord.flags & 0x10) != 0;
            this.checkBox_Flag0x40.IsChecked = (map.worldRecord.flags & 0x40) != 0;
            this.checkBox_Flag0x80.IsChecked = (map.worldRecord.flags & 0x80) != 0;
            this.checkBox_Flag0x100.IsChecked = (map.worldRecord.flags & 0x100) != 0;

            this.comboBox_Type.SelectedIndex = (int)map.worldRecord.type;
            this.textBlock_AssetPath.Text = map.worldRecord.assetPath;
            this.textBox_screenPath.Text = map.worldRecord.screenPath;
            this.textBox_screenModelPath.Text = map.worldRecord.screenModelPath;
            this.textBlock_ChunkBounds0.Text = map.worldRecord.chunkBounds00.ToString();
            this.textBlock_ChunkBounds1.Text = map.worldRecord.chunkBounds01.ToString();
            this.textBlock_ChunkBounds2.Text = map.worldRecord.chunkBounds02.ToString();
            this.textBlock_ChunkBounds3.Text = map.worldRecord.chunkBounds03.ToString();
            this.textBox_plugAverageHeight.Text = map.worldRecord.plugAverageHeight.ToString();
            this.textBox_primeLevelOffset.Text = map.worldRecord.primeLevelOffset.ToString();
            this.textBox_primeLevelMax.Text = map.worldRecord.primeLevelMax.ToString();
            this.textBox_heroismMenaceLevel.Text = map.worldRecord.heroismMenaceLevel.ToString();
        }

        internal void Save()
        {
            if (ProjectManager.project == null) return;
            if (ProjectManager.project.Maps == null) return;

            if (newMap)
            {
                // First create map entry in project, and add to dropdown, and only then fill in the details
                var newMap = new Map();
                newMap.worldRecord = new Map.World();
                //newMap.worldLocations = new List<Map.WorldLocation>();
                newMap.worldRecord.ID = ++ProjectManager.project.lastWorldID;
                ProjectManager.project.Maps.Add(newMap);

                // Add map name to dropdown
                if (this.editor.worldManagerPane.mapIDs == null)
                    this.editor.worldManagerPane.mapIDs = new List<uint>();

                if (this.editor.worldManagerPane.mapNames == null)
                    this.editor.worldManagerPane.mapNames = new System.Collections.ObjectModel.ObservableCollection<string>();

                this.editor.worldManagerPane.mapIDs.Add(newMap.worldRecord.ID);
                this.editor.worldManagerPane.mapNames.Add($"{newMap.worldRecord.ID}. {this.textBox_MapName.Text}");
                //mapRendererPane.mapComboBox.Items.Add(this.textBox_MapName.Text);

                // Select last index
                this.editor.worldManagerPane.mapComboBox.SelectedIndex = this.editor.worldManagerPane.mapComboBox.Items.Count - 1;
            }

            int index = this.editor.worldManagerPane.mapComboBox.SelectedIndex;

            // Add map data to project
            var map = ProjectManager.project.Maps![index];
            var mapName = this.textBox_MapName.Text;

            if (!newMap && map.Name != mapName)
            {
                // Rename map name in dropdown
                this.editor.worldManagerPane.mapNames[index] = mapName;
                //mapRendererPane.mapComboBox.Items[index] = mapName;

                // TODO : Rename map folder and assets like area files and minimaps
            }

            map.Name = mapName;

            if (map.worldRecord != null)
            {
                // ID (non editable)

                // Asset Path
                map.worldRecord.assetPath = $"Map\\{mapName}";

                // Flags
                uint flags = 0;
                if (this.checkBox_Flag0x1.IsChecked == true)
                    flags |= 0x1;
                if (this.checkBox_Flag0x2.IsChecked == true)
                    flags |= 0x2;
                if (this.checkBox_Flag0x4.IsChecked == true)
                    flags |= 0x4;
                if (this.checkBox_Flag0x8.IsChecked == true)
                    flags |= 0x8;
                if (this.checkBox_Flag0x10.IsChecked == true)
                    flags |= 0x10;
                if (this.checkBox_Flag0x40.IsChecked == true)
                    flags |= 0x40;
                if (this.checkBox_Flag0x80.IsChecked == true)
                    flags |= 0x80;
                if (this.checkBox_Flag0x100.IsChecked == true)
                    flags |= 0x100;
                map.worldRecord.flags = flags;

                // Type
                if (this.comboBox_Type.SelectedIndex == -1)
                    this.comboBox_Type.SelectedIndex = 0;
                map.worldRecord.type = (uint)this.comboBox_Type.SelectedIndex;

                // Screen Path
                map.worldRecord.screenPath = this.textBox_screenPath.Text;

                // Screen Model Path
                map.worldRecord.screenModelPath = this.textBox_screenModelPath.Text;

                // Chunk Bounds (calculated not edited)

                // Plug average height
                if (uint.TryParse(this.textBox_plugAverageHeight.Text, out uint plugAverageHeight))
                    map.worldRecord.plugAverageHeight = plugAverageHeight;

                // Localized text ID Name (always 0 in data)
                map.worldRecord.localizedTextIdName = 0;

                // Min/Max item level (always 0 in data)
                map.worldRecord.minItemLevel = 0;
                map.worldRecord.maxItemLevel = 0;

                // Prime Level
                if (uint.TryParse(this.textBox_primeLevelOffset.Text, out uint primeLevelOffset))
                    map.worldRecord.primeLevelOffset = primeLevelOffset;
                if(uint.TryParse(this.textBox_primeLevelMax.Text, out uint primeLevelMax))
                    map.worldRecord.primeLevelMax = primeLevelMax;

                // Veteran Tierl Scaling Type (always 0 in data)
                map.worldRecord.veteranTierScalingType = 0;

                // Heroism Menace Level
                if (uint.TryParse(this.textBox_heroismMenaceLevel.Text, out uint heroismMenaceLevel))
                    map.worldRecord.heroismMenaceLevel = heroismMenaceLevel;

                // Reward Rotation Content ID (always 0 in data)
                map.worldRecord.rewardRotationContentId = 0;
            }

            ProjectManager.SaveProject();
        }

        private void button_Close_Click(object sender, RoutedEventArgs e)
        {
            Close();
        }

        private void button_Save_Click(object sender, RoutedEventArgs e)
        {
            if (this.textBox_MapName.Text == string.Empty || this.textBox_MapName.Text == null)
            {
                System.Windows.MessageBox.Show(this, $"The map name field can not be empty.", "No Name", MessageBoxButton.OK);
            }
            else
            {
                Save();
                Close();
            }
        }
    }
}
