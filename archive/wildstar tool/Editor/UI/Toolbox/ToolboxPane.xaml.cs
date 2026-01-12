using ProjectWS.Editor.Tools;
using System;
using System.Collections.ObjectModel;
using System.Windows;
using System.Windows.Controls;

namespace ProjectWS.Editor.UI.Toolbox
{
    /// <summary>
    /// Interaction logic for UserControl1.xaml
    /// </summary>
    public partial class ToolboxPane : UserControl
    {
        public Engine.Engine? engine;
        public Editor? editor;

        public ToolboxPane()
        {
            InitializeComponent();
        }

        private void UserControl_Loaded(object sender, RoutedEventArgs e)
        {
            
        }

        private void NoToolButton_Checked(object sender, RoutedEventArgs e)
        {
            if (NoToolControl != null)
            {
                NoToolControl.Visibility = Visibility.Visible;
                TerrainSculptControl.Visibility = Visibility.Collapsed;
                TerrainLayerPaintControl.Visibility = Visibility.Collapsed;
                TerrainColorPaintControl.Visibility = Visibility.Collapsed;
                TerrainSkyPaintControl.Visibility = Visibility.Collapsed;
                TerrainPropPlaceControl.Visibility = Visibility.Collapsed;
            }

            for (int i = 0; i < this.editor?.tools?.Count; i++)
            {
                this.editor.tools[i].Disable();
            }
        }

        private void TerrainSculptButton_Checked(object sender, RoutedEventArgs e)
        {
            if (NoToolControl != null)
            {
                NoToolControl.Visibility = Visibility.Collapsed;
                TerrainSculptControl.Visibility = Visibility.Visible;
                TerrainLayerPaintControl.Visibility = Visibility.Collapsed;
                TerrainColorPaintControl.Visibility = Visibility.Collapsed;
                TerrainSkyPaintControl.Visibility = Visibility.Collapsed;
                TerrainPropPlaceControl.Visibility = Visibility.Collapsed;
            }

            TerrainSculptTool? tool = null;

            for (int i = 0; i < this.editor?.tools?.Count; i++)
            {
                if (this.editor.tools[i] is TerrainSculptTool)
                    tool = this.editor.tools[i] as TerrainSculptTool;

                this.editor.tools[i].Disable();
            }

            tool?.Enable();
        }

        private void TerrainLayerPaintToolButton_Checked(object sender, RoutedEventArgs e)
        {
            if (NoToolControl != null)
            {
                NoToolControl.Visibility = Visibility.Collapsed;
                TerrainSculptControl.Visibility = Visibility.Collapsed;
                TerrainLayerPaintControl.Visibility = Visibility.Visible;
                TerrainColorPaintControl.Visibility = Visibility.Collapsed;
                TerrainSkyPaintControl.Visibility = Visibility.Collapsed;
                TerrainPropPlaceControl.Visibility = Visibility.Collapsed;
            }

            TerrainLayerPaintTool? tool = null;

            for (int i = 0; i < this.editor?.tools?.Count; i++)
            {
                if (this.editor.tools[i] is TerrainLayerPaintTool)
                    tool = this.editor.tools[i] as TerrainLayerPaintTool;

                this.editor.tools[i].Disable();
            }

            tool?.Enable();
        }

        private void TerrainColorPaintToolButton_Checked(object sender, RoutedEventArgs e)
        {
            NoToolControl.Visibility = Visibility.Collapsed;
            TerrainSculptControl.Visibility = Visibility.Collapsed;
            TerrainLayerPaintControl.Visibility = Visibility.Collapsed;
            TerrainColorPaintControl.Visibility = Visibility.Visible;
            TerrainSkyPaintControl.Visibility = Visibility.Collapsed;
            TerrainPropPlaceControl.Visibility = Visibility.Collapsed;

            for (int i = 0; i < this.editor?.tools?.Count; i++)
            {
                this.editor.tools[i].Disable();
            }
        }

        private void SkyPaintToolButton_Checked(object sender, RoutedEventArgs e)
        {
            NoToolControl.Visibility = Visibility.Collapsed;
            TerrainSculptControl.Visibility = Visibility.Collapsed;
            TerrainLayerPaintControl.Visibility = Visibility.Collapsed;
            TerrainColorPaintControl.Visibility = Visibility.Collapsed;
            TerrainSkyPaintControl.Visibility = Visibility.Visible;
            TerrainPropPlaceControl.Visibility = Visibility.Collapsed;

            for (int i = 0; i < this.editor?.tools?.Count; i++)
            {
                this.editor.tools[i].Disable();
            }
        }

        private void PropToolButton_Checked(object sender, RoutedEventArgs e)
        {
            NoToolControl.Visibility = Visibility.Collapsed;
            TerrainSculptControl.Visibility = Visibility.Collapsed;
            TerrainLayerPaintControl.Visibility = Visibility.Collapsed;
            TerrainColorPaintControl.Visibility = Visibility.Collapsed;
            TerrainSkyPaintControl.Visibility = Visibility.Collapsed;
            TerrainPropPlaceControl.Visibility = Visibility.Visible;

            PropTool? tool = null;

            for (int i = 0; i < this.editor?.tools?.Count; i++)
            {
                if (this.editor.tools[i] is PropTool)
                    tool = this.editor.tools[i] as PropTool;

                this.editor.tools[i].Disable();
            }

            tool?.Enable();
        }
    }
}
