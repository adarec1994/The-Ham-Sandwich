using MathUtils;
using ProjectWS.Engine.Data;
using ProjectWS.FileFormats.Sky;
using System;
using System.Collections.ObjectModel;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Media;

namespace ProjectWS.Editor
{
    /// <summary>
    /// Interaction logic for UserControl1.xaml
    /// </summary>
    public partial class SkyEditorPane : UserControl
    {
        public Action<int>? changeSkyID;
        public ObservableCollection<ProjectWS.Engine.Database.Definitions.WorldSky>? skies { get; set; }
        public Engine.Engine? engine;
        public TimeTrackColorPane subWindow;
        public int selectedTime;

        public SkyEditorPane()
        {
            InitializeComponent();

            this.skyDataGrid.DataContext = this;
            this.skies = new ObservableCollection<ProjectWS.Engine.Database.Definitions.WorldSky>();
        }

        private void UserControl_Loaded(object sender, RoutedEventArgs e)
        {
            
        }

        private void skyDataGrid_SelectionChanged(object sender, SelectionChangedEventArgs e)
        {
            if (this.engine == null) return;

            var selectedWorldSkyRow = (ProjectWS.Engine.Database.Definitions.WorldSky)this.skyDataGrid.SelectedItem;

            var sky = this.engine.GetSky(selectedWorldSkyRow.ID);

            PopulateTreeView(this, sky);
        }

        void PopulateTreeView(SkyEditorPane pane, File sky)
        {
            pane.skyTreeView.Items.Clear();

            int time = 21;

            // Source Path //
            TreeViewItem sourcePath = new TreeViewItem();
            sourcePath.Header = "Source Path";
            TreeViewItem sourcePath_path = new TreeViewItem();
            sourcePath_path.Header = sky.sourceFilePath;
            sourcePath.Items.Add(sourcePath_path);
            pane.skyTreeView.Items.Add(sourcePath);

            // Sky Data Blocks //
            for (int i = 0; i < 4; i++)
            {
                PopulateSkyDataBlock(pane, sky, time, i);
            }

            // Sun Light Color //
            if (sky.sunLightColor != null && sky.sunLightColor.timestamps != null)// && sky.sunLightColor.timestamps.Length > 0)
            {
                var name = $"Sun Diffuse [{sky.sunLightColor.timestamps.Length}]";
                TreeViewItem treeItem = new TreeViewItem();
                treeItem.Header = name;
                treeItem.MouseLeftButtonUp += (sender, e) => { OpenColorWindow(name, time, sky.sunLightColor); };
                pane.skyTreeView.Items.Add(treeItem);
            }

            // Sun Specular Color //
            if (sky.specularColor != null && sky.specularColor.timestamps != null)// && sky.specularColor.timestamps.Length > 0)
            {
                var name = $"Sun Specular [{sky.specularColor.timestamps.Length}]";
                TreeViewItem treeItem = new TreeViewItem();
                treeItem.Header = name;
                treeItem.MouseLeftButtonUp += (sender, e) => { OpenAngleAndColorWindow(name, time, sky.specularColor); };
                pane.skyTreeView.Items.Add(treeItem);
            }

            // unkColorData //
            if (sky.unkColorData != null && sky.unkColorData.timestamps != null)// && sky.unkColorData.timestamps.Length > 0)
            {
                var name = $"unkColorData [{sky.unkColorData.timestamps.Length}]";
                TreeViewItem treeItem = new TreeViewItem();
                treeItem.Header = name;
                treeItem.MouseLeftButtonUp += (sender, e) => { OpenColorWindow(name, time, sky.unkColorData); };
                pane.skyTreeView.Items.Add(treeItem);
            }

            // fogSettings //
            if (sky.fogSettings != null && sky.fogSettings.timestamps != null)// && sky.fogSettings.timestamps.Length > 0)
            {
                var name = $"fogSettings [{sky.fogSettings.timestamps.Length}]";
                TreeViewItem treeItem = new TreeViewItem();
                treeItem.Header = name;
                treeItem.MouseLeftButtonUp += (sender, e) => { OpenFogSettingsWindow(name, time, sky.fogSettings); };
                pane.skyTreeView.Items.Add(treeItem);
            }

            // skyboxModels //
            if (sky.skyboxModels != null)// && sky.skyboxModels.Length > 0)
            {
                TreeViewItem skyBoxModels = new TreeViewItem();
                skyBoxModels.Header = "Skybox Models";

                for (int i = 0; i < sky.skyboxModels.Length; i++)
                {
                    TreeViewItem skyBoxModels_path = new TreeViewItem();
                    skyBoxModels_path.Header = sky.skyboxModels[i].filePath;
                    skyBoxModels.Items.Add(skyBoxModels_path);
                }
                pane.skyTreeView.Items.Add(skyBoxModels);
            }

            // postFXSettings //
            if (sky.postFXSettings != null && sky.postFXSettings.timestamps != null)// && sky.fogSettings.timestamps.Length > 0)
            {
                var name = $"postFXSettings [{sky.postFXSettings.timestamps.Length}]";
                TreeViewItem treeItem = new TreeViewItem();
                treeItem.Header = name;
                treeItem.MouseLeftButtonUp += (sender, e) => { OpenPostFXWindow(name, time, sky.postFXSettings); };
                pane.skyTreeView.Items.Add(treeItem);
            }

            // unk9 //
            if (sky.unk9 != null && sky.unk9.timestamps != null)// && sky.fogSettings.timestamps.Length > 0)
            {
                var name = $"unk9 [{sky.unk9.timestamps.Length}]";
                TreeViewItem treeItem = new TreeViewItem();
                treeItem.Header = name;
                treeItem.MouseLeftButtonUp += (sender, e) => { OpenIntWindow(name, time, sky.unk9); };
                pane.skyTreeView.Items.Add(treeItem);
            }

            // unk10 //
            if (sky.unk10 != null && sky.unk10.timestamps != null)// && sky.fogSettings.timestamps.Length > 0)
            {
                var name = $"unk9 [{sky.unk10.timestamps.Length}]";
                TreeViewItem treeItem = new TreeViewItem();
                treeItem.Header = name;
                treeItem.MouseLeftButtonUp += (sender, e) => { OpenIntWindow(name, time, sky.unk10); };
                pane.skyTreeView.Items.Add(treeItem);
            }

            // particulates //
            if (sky.particulates != null)// && sky.skyboxModels.Length > 0)
            {
                TreeViewItem particulates = new TreeViewItem();
                particulates.Header = "Particulates";

                for (int i = 0; i < sky.particulates.Length; i++)
                {
                    TreeViewItem particulates_path = new TreeViewItem();
                    particulates_path.Header = sky.particulates[i];
                    particulates.Items.Add(particulates_path);
                }
                pane.skyTreeView.Items.Add(particulates);
            }

            // environmentMapPath //
            TreeViewItem environmentMapPath = new TreeViewItem();
            TreeViewItem environmentMapPath_path = new TreeViewItem();
            environmentMapPath.Header = "Environment Map";
            environmentMapPath_path.Header = sky.environmentMapPath;
            environmentMapPath.Items.Add(environmentMapPath_path);
            pane.skyTreeView.Items.Add(environmentMapPath);

            // unk13 //
            if (sky.unk13 != null && sky.unk13.timestamps != null)// && sky.unkColorData.timestamps.Length > 0)
            {
                var name = $"Unk Color 13 [{sky.unk13.timestamps.Length}]";
                TreeViewItem treeItem = new TreeViewItem();
                treeItem.Header = name;
                treeItem.MouseLeftButtonUp += (sender, e) => { OpenColorWindow(name, time, sky.unk13); };
                pane.skyTreeView.Items.Add(treeItem);
            }

            // sunGlarePathA //
            TreeViewItem sunGlarePathA = new TreeViewItem();
            TreeViewItem sunGlarePathA_path = new TreeViewItem();
            sunGlarePathA.Header = "Sun Glare Map A";
            sunGlarePathA_path.Header = sky.sunGlarePathA;
            sunGlarePathA.Items.Add(sunGlarePathA_path);
            pane.skyTreeView.Items.Add(sunGlarePathA);

            // sunGlarePathA //
            TreeViewItem sunGlarePathB = new TreeViewItem();
            TreeViewItem sunGlarePathB_path = new TreeViewItem();
            sunGlarePathB.Header = "Sun Glare Map B";
            sunGlarePathB_path.Header = sky.sunGlarePathB;
            sunGlarePathB.Items.Add(sunGlarePathB_path);
            pane.skyTreeView.Items.Add(sunGlarePathB);

            // unk16 //
            if (sky.unk16 != null && sky.unk16.timestamps != null)
            {
                var name = $"Unk Float 16 [{sky.unk16.timestamps.Length}]";
                TreeViewItem treeItem = new TreeViewItem();
                treeItem.Header = name;
                treeItem.MouseLeftButtonUp += (sender, e) => { OpenFloatWindow(name, time, sky.unk16); };
                pane.skyTreeView.Items.Add(treeItem);
            }

            // unk17 //
            if (sky.unk17 != null && sky.unk17.timestamps != null)
            {
                var name = $"Unk Color 17 [{sky.unk17.timestamps.Length}]";
                TreeViewItem treeItem = new TreeViewItem();
                treeItem.Header = name;
                treeItem.MouseLeftButtonUp += (sender, e) => { OpenColorWindow(name, time, sky.unk17); };
                pane.skyTreeView.Items.Add(treeItem);
            }

            // unk18 //
            if (sky.unk18 != null && sky.unk18.timestamps != null)
            {
                var name = $"Unk Color 18 [{sky.unk18.timestamps.Length}]";
                TreeViewItem treeItem = new TreeViewItem();
                treeItem.Header = name;
                treeItem.MouseLeftButtonUp += (sender, e) => { OpenColorWindow(name, time, sky.unk18); };
                pane.skyTreeView.Items.Add(treeItem);
            }

            // unk19 //
            if (sky.unk19 != null && sky.unk19.timestamps != null)
            {
                var name = $"Unk ColorAB 19 [{sky.unk19.timestamps.Length}]";
                TreeViewItem treeItem = new TreeViewItem();
                treeItem.Header = name;
                treeItem.MouseLeftButtonUp += (sender, e) => { OpenColorABWindow(name, time, sky.unk19); };
                pane.skyTreeView.Items.Add(treeItem);
            }

            // unk20 //
            if (sky.unk20 != null && sky.unk20.timestamps != null)
            {
                var name = $"Unk ColorAB 20 [{sky.unk20.timestamps.Length}]";
                TreeViewItem treeItem = new TreeViewItem();
                treeItem.Header = name;
                treeItem.MouseLeftButtonUp += (sender, e) => { OpenColorABWindow(name, time, sky.unk20); };
                pane.skyTreeView.Items.Add(treeItem);
            }

            // unk21 //
            if (sky.unk21 != null && sky.unk21.timestamps != null)
            {
                var name = $"Unk ColorAB 21 [{sky.unk21.timestamps.Length}]";
                TreeViewItem treeItem = new TreeViewItem();
                treeItem.Header = name;
                treeItem.MouseLeftButtonUp += (sender, e) => { OpenColorABWindow(name, time, sky.unk21); };
                pane.skyTreeView.Items.Add(treeItem);
            }


            // unk22 //
            if (sky.unk22 != null && sky.unk22.timestamps != null)
            {
                var name = $"Unk ColorAB 22 [{sky.unk22.timestamps.Length}]";
                TreeViewItem treeItem = new TreeViewItem();
                treeItem.Header = name;
                treeItem.MouseLeftButtonUp += (sender, e) => { OpenColorABWindow(name, time, sky.unk22); };
                pane.skyTreeView.Items.Add(treeItem);
            }

            // unk23 //
            if (sky.unk23 != null && sky.unk23.timestamps != null)
            {
                var name = $"Unk ColorAB 23 [{sky.unk23.timestamps.Length}]";
                TreeViewItem treeItem = new TreeViewItem();
                treeItem.Header = name;
                treeItem.MouseLeftButtonUp += (sender, e) => { OpenColorABWindow(name, time, sky.unk23); };
                pane.skyTreeView.Items.Add(treeItem);
            }

            // unk24 //
            if (sky.unk24 != null && sky.unk24.timestamps != null)
            {
                var name = $"Unk ColorAB 24 [{sky.unk24.timestamps.Length}]";
                TreeViewItem treeItem = new TreeViewItem();
                treeItem.Header = name;
                treeItem.MouseLeftButtonUp += (sender, e) => { OpenColorABWindow(name, time, sky.unk24); };
                pane.skyTreeView.Items.Add(treeItem);
            }

            // unk25 //
            if (sky.unk25 != null && sky.unk25.timestamps != null)
            {
                var name = $"Unk ColorAB 25 [{sky.unk25.timestamps.Length}]";
                TreeViewItem treeItem = new TreeViewItem();
                treeItem.Header = name;
                treeItem.MouseLeftButtonUp += (sender, e) => { OpenColorABWindow(name, time, sky.unk25); };
                pane.skyTreeView.Items.Add(treeItem);
            }

            // unk26 //
            if (sky.unk26 != null && sky.unk26.timestamps != null)
            {
                var name = $"Unk ColorAB 22 [{sky.unk26.timestamps.Length}]";
                TreeViewItem treeItem = new TreeViewItem();
                treeItem.Header = name;
                treeItem.MouseLeftButtonUp += (sender, e) => { OpenColorABWindow(name, time, sky.unk26); };
                pane.skyTreeView.Items.Add(treeItem);
            }

            // UnkData.. idk //

            // Lut File //
            TreeViewItem lutPath = new TreeViewItem();
            TreeViewItem lutPath_path = new TreeViewItem();
            lutPath.Header = "Lut Path";
            lutPath_path.Header = sky.lutFile;
            lutPath.Items.Add(lutPath_path);
            pane.skyTreeView.Items.Add(lutPath);

        }

        void PopulateSkyDataBlock(SkyEditorPane pane, File sky, int time, int index)
        {
            var data = sky.skyDataBlock0;

            /*
            TreeViewItem block = new TreeViewItem();
            block.Header = "Block " + index;
            */
            // Unk //
            /*
            TreeViewItem unk = new TreeViewItem();
            unk.Header = "Unk: " + data.unk;
            block.Items.Add(unk);
            */

            // ColorUnk0 //
            if (data.colorUnk0 != null)
            {
                for (int i = 0; i < data.colorUnk0.Length; i++)
                {
                    var idx = i;

                    if (data.colorUnk0[idx] != null && data.colorUnk0[idx].timestamps != null)
                    {
                        var name = $"Unk Color {i} [{data.colorUnk0[idx].timestamps.Length}]";
                        TreeViewItem treeItem = new TreeViewItem();
                        treeItem.Header = name;
                        treeItem.MouseLeftButtonUp += (sender, e) => { OpenColorWindow(name, time, data.colorUnk0[idx]); };
                        pane.skyTreeView.Items.Add(treeItem);
                    }
                }
            }

            // ColorAndAngleUnk0 //
            if (data.colorAndAngleUnk0 != null)
            {
                for (int i = 0; i < data.colorAndAngleUnk0.Length; i++)
                {
                    var idx = i;
                    var name = $"Unk Angle + Color {i} [{data.colorAndAngleUnk0[idx].timestamps.Length}]";
                    TreeViewItem treeItem = new TreeViewItem();
                    treeItem.Header = name;
                    treeItem.MouseLeftButtonUp += (sender, e) => { OpenAngleAndColorWindow(name, time, data.colorAndAngleUnk0[idx]); };
                    pane.skyTreeView.Items.Add(treeItem);
                }
            }

            // UnkGradient2_0
            if (data.unkColorABAngle != null)
            {
                for (int i = 0; i < data.unkColorABAngle.Length; i++)
                {
                    var idx = i;
                    var name = $"Unk ColorAB + Angle {i} [{data.unkColorABAngle[idx].timestamps.Length}]";
                    TreeViewItem treeItem = new TreeViewItem();
                    treeItem.Header = name;
                    treeItem.MouseLeftButtonUp += (sender, e) => { OpenAngleAndColorABWindow(name, time, data.unkColorABAngle[idx]); };
                    pane.skyTreeView.Items.Add(treeItem);
                }
            }

            // UnkColorBlock3
            if (data.angleABAndColor != null)
            {
                for (int i = 0; i < data.angleABAndColor.Length; i++)
                {
                    var idx = i;
                    var name = $"Unk AngleAB + Color {i} [{data.angleABAndColor[idx].timestamps.Length}]";
                    TreeViewItem treeItem = new TreeViewItem();
                    treeItem.Header = name;
                    treeItem.MouseLeftButtonUp += (sender, e) => { OpenAngleABAndColorWindow(name, time, data.angleABAndColor[idx]); };
                    pane.skyTreeView.Items.Add(treeItem);
                }
            }

            // UnkColorBlock7
            if (data.shCoefficients != null)
            {
                for (int i = 0; i < data.shCoefficients.Length; i++)
                {
                    var idx = i;
                    var name = $"Unk7 {i} [{data.shCoefficients[idx].timestamps.Length}]";
                    TreeViewItem treeItem = new TreeViewItem();
                    treeItem.Header = name;
                    treeItem.MouseLeftButtonUp += (sender, e) => { OpenUnkBlockWindow(name, time, data.shCoefficients[idx]); };
                    pane.skyTreeView.Items.Add(treeItem);
                }
            }

            // skySphereGradient
            if (data.skySphereGradient != null && data.skySphereGradient.timestamps.Length > 0)
            {
                var name = $"Sky Gradient [{data.skySphereGradient.timestamps.Length}]";
                TreeViewItem treeItem = new TreeViewItem();
                treeItem.Header = name;
                treeItem.MouseLeftButtonUp += (sender, e) => { OpenGradient16Window(name, time, data.skySphereGradient); };
                pane.skyTreeView.Items.Add(treeItem);
            }

            //pane.skyTreeView.Items.Add(block);
        }

        void OpenSubwindow(string text)
        {
            if (this.subWindow == null)
            {
                this.subWindow = new TimeTrackColorPane();
                this.subWindow.Closing += (sender, e) => { this.subWindow.Visibility = Visibility.Hidden; e.Cancel = true; };
                this.subWindow.Owner = Program.mainWindow;
                this.subWindow.timeSlider.ValueChanged += ChangeTime;
                this.subWindow.Background = new SolidColorBrush(System.Windows.Media.Color.FromRgb(0, 0, 0));
            }
            this.subWindow.Title = text;
            this.subWindow.Show();
        }

        void OpenColorWindow(string text, int time, TimeTrack<Vector4> timeTrack)
        {
            int timeIdx = FindClosestTimeIndex(time, timeTrack.timestamps);
            OpenSubwindow($"{text} {TimeStampToTimeString(timeTrack.timestamps[timeIdx])}");

            // Populate with values //
            this.subWindow.listBox.Items.Clear();
            var picker = CreateColorPicker(timeTrack.data[timeIdx]);
            this.subWindow.listBox.Items.Add(picker);
        }

        void OpenAngleAndColorWindow(string text, int time, TimeTrack<AngleAndColor> timeTrack)
        {
            int timeIdx = FindClosestTimeIndex(time, timeTrack.timestamps);
            OpenSubwindow($"{text} {TimeStampToTimeString(timeTrack.timestamps[timeIdx])}");

            // Populate with values //
            this.subWindow.listBox.Items.Clear();
            var picker = CreateColorPicker(timeTrack.data[timeIdx].color);
            this.subWindow.listBox.Items.Add(picker);
            {
                // TODO : Add angle
            }
        }

        void OpenAngleAndColorABWindow(string text, int time, TimeTrack<AngleAndColorAB> timeTrack)
        {
            int timeIdx = FindClosestTimeIndex(time, timeTrack.timestamps);
            OpenSubwindow($"{text} {TimeStampToTimeString(timeTrack.timestamps[timeIdx])}");

            // Populate with values //
            this.subWindow.listBox.Items.Clear();

            var pickerA = CreateColorPicker(timeTrack.data[timeIdx].colorA);
            this.subWindow.listBox.Items.Add(pickerA);

            var pickerB = CreateColorPicker(timeTrack.data[timeIdx].colorB);
            this.subWindow.listBox.Items.Add(pickerB);

            // TODO : Add angle
        }

        void OpenColorABWindow(string text, int time, TimeTrack<ColorAB> timeTrack)
        {
            int timeIdx = FindClosestTimeIndex(time, timeTrack.timestamps);
            OpenSubwindow($"{text} {TimeStampToTimeString(timeTrack.timestamps[timeIdx])}");

            // Populate with values //
            this.subWindow.listBox.Items.Clear();

            var pickerA = CreateColorPicker(timeTrack.data[timeIdx].colorA);
            this.subWindow.listBox.Items.Add(pickerA);

            var pickerB = CreateColorPicker(timeTrack.data[timeIdx].colorB);
            this.subWindow.listBox.Items.Add(pickerB);
        }

        void OpenGradient16Window(string text, int time, TimeTrack<Gradient16> timeTrack)
        {
            int timeIdx = FindClosestTimeIndex(time, timeTrack.timestamps);
            OpenSubwindow($"{text} {TimeStampToTimeString(timeTrack.timestamps[timeIdx])}");

            // Populate with values //
            this.subWindow.listBox.Items.Clear();

            for (int j = 0; j < timeTrack.data[timeIdx].colors.Length; j++)
            {
                var pickerA = CreateColorPicker(timeTrack.data[timeIdx].colors[j]);
                this.subWindow.listBox.Items.Add(pickerA);
            }
        }

        void OpenAngleABAndColorWindow(string text, int time, TimeTrack<AngleABAndColor> timeTrack)
        {
            int timeIdx = FindClosestTimeIndex(time, timeTrack.timestamps);
            OpenSubwindow($"{text} {TimeStampToTimeString(timeTrack.timestamps[timeIdx])}");

            // Populate with values //
            this.subWindow.listBox.Items.Clear();

            var pickerA = CreateColorPicker(timeTrack.data[timeIdx].color);
            this.subWindow.listBox.Items.Add(pickerA);

            // TODO : Add angle A and B
        }

        void OpenUnkBlockWindow(string text, int time, TimeTrack<SHCoefficients> timeTrack)
        {
            int timeIdx = FindClosestTimeIndex(time, timeTrack.timestamps);
            OpenSubwindow($"{text} {TimeStampToTimeString(timeTrack.timestamps[timeIdx])}");

            // Populate with values //
            this.subWindow.listBox.Items.Clear();

            //var pickerA = CreateColorPicker(timeTrack.data[timeIdx].unk2);
            //this.subWindow.listBox.Items.Add(pickerA);

            // TODO : This has a lot of shit, maybe not colors
        }

        void OpenFogSettingsWindow(string text, int time, TimeTrack<FogSettings> timeTrack)
        {
            int timeIdx = FindClosestTimeIndex(time, timeTrack.timestamps);
            OpenSubwindow($"{text} {TimeStampToTimeString(timeTrack.timestamps[timeIdx])}");

            // Populate with values //
            this.subWindow.listBox.Items.Clear();

            var unk0 = CreateFloatInput("Unk 0", timeTrack.data[timeIdx].unk0);
            this.subWindow.listBox.Items.Add(unk0);

            var fogStartDistance = CreateFloatInput("Fog Start Distance", timeTrack.data[timeIdx].fogStartDistance);
            this.subWindow.listBox.Items.Add(fogStartDistance);

            var unk1 = CreateFloatInput("Unk 1", timeTrack.data[timeIdx].unk1);
            this.subWindow.listBox.Items.Add(unk1);

            var unk2 = CreateFloatInput("Unk 2", timeTrack.data[timeIdx].unk2);
            this.subWindow.listBox.Items.Add(unk2);

            var unk3 = CreateFloatInput("Unk 3", timeTrack.data[timeIdx].unk3);
            this.subWindow.listBox.Items.Add(unk3);
        }

        void OpenPostFXWindow(string text, int time, TimeTrack<PostFXSettings> timeTrack)
        {
            int timeIdx = FindClosestTimeIndex(time, timeTrack.timestamps);
            OpenSubwindow($"{text} {TimeStampToTimeString(timeTrack.timestamps[timeIdx])}");

            // Populate with values //
            this.subWindow.listBox.Items.Clear();

            var overlayColor = CreateColorPicker(timeTrack.data[timeIdx].overlayColor);
            this.subWindow.listBox.Items.Add(overlayColor);

            var unk0 = CreateFloatInput("Unk 0", timeTrack.data[timeIdx].unk0);
            this.subWindow.listBox.Items.Add(unk0);

            var unk1 = CreateFloatInput("Unk 1", timeTrack.data[timeIdx].unk1);
            this.subWindow.listBox.Items.Add(unk1);

            var finalImageSaturation = CreateFloatInput("Image Saturation", timeTrack.data[timeIdx].finalImageSaturation);
            this.subWindow.listBox.Items.Add(finalImageSaturation);

            var inverseColorOverlay = CreateFloatInput("Inverse Color Overlay", timeTrack.data[timeIdx].inverseColorOverlay);
            this.subWindow.listBox.Items.Add(inverseColorOverlay);

            var brightness = CreateFloatInput("Brightness", timeTrack.data[timeIdx].brightness);
            this.subWindow.listBox.Items.Add(brightness);

            var inverseExposure = CreateFloatInput("Inverse Exposure", timeTrack.data[timeIdx].inverseExposure);
            this.subWindow.listBox.Items.Add(inverseExposure);

            var unk2 = CreateFloatInput("Unk 2", timeTrack.data[timeIdx].unk2);
            this.subWindow.listBox.Items.Add(unk2);

            var unk3 = CreateFloatInput("Unk 3", timeTrack.data[timeIdx].unk3);
            this.subWindow.listBox.Items.Add(unk3);

            var unk4 = CreateFloatInput("Unk 4", timeTrack.data[timeIdx].unk4);
            this.subWindow.listBox.Items.Add(unk4);

            var unk5 = CreateFloatInput("Unk 5", timeTrack.data[timeIdx].unk5);
            this.subWindow.listBox.Items.Add(unk5);

            var unk6 = CreateFloatInput("Unk 6", timeTrack.data[timeIdx].unk6);
            this.subWindow.listBox.Items.Add(unk6);

            var inverseGamma = CreateFloatInput("Inverse Gamma", timeTrack.data[timeIdx].inverseGamma);
            this.subWindow.listBox.Items.Add(inverseGamma);

            var bloomAlpha = CreateFloatInput("Bloom Alpha", timeTrack.data[timeIdx].bloomAlpha);
            this.subWindow.listBox.Items.Add(bloomAlpha);

            var bloomStrength = CreateFloatInput("Bloom Strength", timeTrack.data[timeIdx].bloomStrength);
            this.subWindow.listBox.Items.Add(bloomStrength);

            var unk7 = CreateFloatInput("Unk 7", timeTrack.data[timeIdx].unk7);
            this.subWindow.listBox.Items.Add(unk7);
            var unk8 = CreateFloatInput("Unk 8", timeTrack.data[timeIdx].unk8);
            this.subWindow.listBox.Items.Add(unk8);
            var unk9 = CreateFloatInput("Unk 9", timeTrack.data[timeIdx].unk9);
            this.subWindow.listBox.Items.Add(unk9);
            var unk10 = CreateFloatInput("Unk 10", timeTrack.data[timeIdx].unk10);
            this.subWindow.listBox.Items.Add(unk10);
            var unk11 = CreateFloatInput("Unk 11", timeTrack.data[timeIdx].unk11);
            this.subWindow.listBox.Items.Add(unk11);
            var unk12 = CreateFloatInput("Unk 12", timeTrack.data[timeIdx].unk12);
            this.subWindow.listBox.Items.Add(unk12);
        }

        void OpenIntWindow(string text, int time, TimeTrack<int> timeTrack)
        {
            int timeIdx = FindClosestTimeIndex(time, timeTrack.timestamps);
            OpenSubwindow($"{text} {TimeStampToTimeString(timeTrack.timestamps[timeIdx])}");

            // Populate with values //
            this.subWindow.listBox.Items.Clear();

            var unk0 = CreateFloatInput("Unk", timeTrack.data[timeIdx]);
            this.subWindow.listBox.Items.Add(unk0);
        }
        void OpenFloatWindow(string text, int time, TimeTrack<float> timeTrack)
        {
            int timeIdx = FindClosestTimeIndex(time, timeTrack.timestamps);
            OpenSubwindow($"{text} {TimeStampToTimeString(timeTrack.timestamps[timeIdx])}");

            // Populate with values //
            this.subWindow.listBox.Items.Clear();

            var unk0 = CreateFloatInput("Unk", timeTrack.data[timeIdx]);
            this.subWindow.listBox.Items.Add(unk0);
        }


        ColorPicker.PortableColorPicker CreateColorPicker(Vector4 col)
        {
            //ColorPicker.StandardColorPicker stPicker = new ColorPicker.StandardColorPicker();
            //stPicker.Width = 200;
            //stPicker.Height = 380;

            ColorPicker.PortableColorPicker picker = new ColorPicker.PortableColorPicker();
            //Binding myBinding = new Binding("ElementName");
            //myBinding.Source = stPicker;
            //picker.SetBinding(ColorPicker.PortableColorPicker.ColorStateProperty, myBinding);

            picker.ColorState = new ColorPicker.Models.ColorState() { RGB_R = col.X, RGB_G = col.Y, RGB_B = col.Z, A = 1.0f };
            picker.Height = 30;

            return picker;
        }

        TimeTrackFloatInput CreateFloatInput(string name, float defaultValue)
        {
            TimeTrackFloatInput tb = new TimeTrackFloatInput();
            tb.inputField.Text = defaultValue.ToString();
            tb.label.Content = name;
            return tb;
        }

        private void ChangeTime(object sender, RoutedPropertyChangedEventArgs<double> e)
        {
            var dValue = e.NewValue;

            this.selectedTime = (int)dValue;
        }

        int FindClosestTimeIndex(int time, uint[] arr)
        {
            if (arr.Length == 0) return 0;

            var target = time * 3600;

            int n = arr.Length;

            // Corner cases
            if (target <= arr[0])
                return 0;
            if (target >= arr[n - 1])
                return n - 1;

            // Doing binary search
            int i = 0, j = n, mid = 0;
            while (i < j)
            {
                mid = (i + j) / 2;

                if (arr[mid] == target)
                    return mid;

                /* If target is less
                than array element,
                then search in left */
                if (target < arr[mid])
                {

                    // If target is greater
                    // than previous to mid,
                    // return closest of two
                    if (mid > 0 && target > arr[mid - 1])
                        return GetClosest(arr[mid - 1], arr[mid], mid - 1, mid, target);

                    /* Repeat for left half */
                    j = mid;
                }

                // If target is
                // greater than mid
                else
                {
                    if (mid < n - 1 && target < arr[mid + 1])
                        return GetClosest(arr[mid], arr[mid + 1], mid, mid + 1, target);
                    i = mid + 1; // update i
                }
            }

            // Only single element
            // left after search
            return mid;
        }

        int GetClosest(uint val1, uint val2, int idx1, int idx2, int target)
        {
            if (target - val1 >= val2 - target)
                return idx2;
            else
                return idx1;
        }

        string TimeStampToTimeString(uint timeStamp)
        {
            float h = ((float)timeStamp / 86400.0f) * 24.0f;
            float m = 60f - (((float)Math.Ceiling(h) - h) * 60f);
            if (m == 60f) m = 0f;
            return $"{h}:{m}";
        }
    }
}
