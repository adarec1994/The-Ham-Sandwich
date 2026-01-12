using ProjectWS.Engine.Data;
using System;
using System.Collections.Generic;
using System.Collections.ObjectModel;
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

namespace ProjectWS.Editor.UI
{
    /// <summary>
    /// Interaction logic for MapImportWindow.xaml
    /// </summary>
    public partial class MapImportWindow : Window
    {
        public Editor editor { get; }
        public ObservableCollection<string>? mapList;
        public List<uint>? mapIDs;

        public MapImportWindow(Editor editor)
        {
            InitializeComponent();
            this.editor = editor;

            this.mapList = new ObservableCollection<string>();
            this.mapIDs = new List<uint>();
            if (DataManager.database?.world != null)
            {
                foreach (var item in DataManager.database.world.records)
                {
                    this.mapList.Add($"{item.Key}. {System.IO.Path.GetFileName(item.Value.assetPath)}");
                    this.mapIDs.Add(item.Key);
                }
            }
            comboBox_GameDataMapList.ItemsSource = mapList;
        }

        private void button_Import_Click(object sender, RoutedEventArgs e)
        {
            if (radioButton_gameData.IsChecked == true)
            {
                if (this.comboBox_GameDataMapList.SelectedIndex != -1)
                    this.editor.ImportGameMap(DataManager.database.world.Get(this.mapIDs[this.comboBox_GameDataMapList.SelectedIndex]));
            }
            else if (radioButton_localData.IsChecked == true)
            {
                this.editor.ImportLocalMap(this.textBox_localDataPath.Text);
            }

            Close();
        }

        private void button_Browse_Click(object sender, RoutedEventArgs e)
        {

        }
    }
}
