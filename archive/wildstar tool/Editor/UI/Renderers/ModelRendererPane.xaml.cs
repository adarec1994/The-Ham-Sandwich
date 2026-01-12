using OpenTK.Wpf;
using ProjectWS.Engine.Mesh;
using ProjectWS.Engine.Rendering;
using System;
using System.Collections.ObjectModel;
using System.Windows;
using System.Windows.Controls;
using static OpenTK.Graphics.OpenGL.GL;

namespace ProjectWS.Editor
{
    /// <summary>
    /// Interaction logic for UserControl1.xaml
    /// </summary>
    public partial class ModelRendererPane : UserControl
    {
        public Action<int>? changeRenderMode;

        public ObservableCollection<string>? renderModes { get; set; }

        public ModelRendererPane()
        {
            InitializeComponent();

            this.renderModeComboBox.DataContext = this;
            this.renderModes = new ObservableCollection<string>();

            foreach (Renderer.ShadingOverride shading in (Renderer.ShadingOverride[])Enum.GetValues(typeof(Renderer.ShadingOverride)))
            {
                this.renderModes.Add(shading.ToString());
            }

            this.renderModeComboBox.SelectedIndex = 0;
        }

        public GLWpfControl GetOpenTKControl()
        {
            return this.GLWpfControl;
        }

        public Grid GetRendererGrid()
        {
            return this.RendererGrid;
        }

        private void renderModeComboBox_SelectionChanged(object sender, SelectionChangedEventArgs e)
        {
            this.renderModeComboBox.Focusable = false;

            if (changeRenderMode != null)
                changeRenderMode.Invoke(this.renderModeComboBox.SelectedIndex);
        }

        private void UserControl_Loaded(object sender, RoutedEventArgs e)
        {

        }

        private void renderModeComboBox_Loaded(object sender, RoutedEventArgs e)
        {

        }

        public void UpdateModelInformation(Engine.Objects.M3Model model)
        {
            for (int g = 0; g < model?.geometries?.Length; g++)
            {
                var geometryIndex = g;

                for (int m = 0; m < model.geometries[g].meshes?.Length; m++)
                {
                    var meshIndex = m;

                    CheckBox checkBox = new CheckBox();

                    if (model?.geometries?[g]?.meshes?[m] != null)
                        checkBox.IsChecked = model.geometries[g].meshes[m].renderable;
                    else
                        checkBox.IsChecked = false;

                    checkBox.Checked += (object sender, RoutedEventArgs e) => { 
                        ToggleMesh(model, geometryIndex, meshIndex, true);
                    };
                    checkBox.Unchecked += (object sender, RoutedEventArgs e) => {
                        ToggleMesh(model, geometryIndex, meshIndex, false);
                    };

                    checkBox.Name = $"G{g}M{m}";
                    checkBox.Content = $"G{g}M{m}";

                    this.StackPanel_Meshes.Children.Add(checkBox);
                }
            }
        }

        private void ToggleMesh(Engine.Objects.M3Model model, int geometry, int mesh, bool? on)
        {
            if (on != null)
            {
                if (model?.geometries?[geometry]?.meshes?[mesh] != null)
                {
                    model.geometries[geometry].meshes[mesh].renderable = (bool)on;     
                }
            }
        }
    }
}
