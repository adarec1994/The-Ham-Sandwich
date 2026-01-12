using ProjectWS.Engine;
using System;
using System.IO;
using System.Windows;
using System.Windows.Controls;

namespace ProjectWS.Editor.UI
{
    /// <summary>
    /// Interaction logic for DataManagerWindow.xaml
    /// </summary>
    public partial class DataManagerWindow : Window
    {
        Editor editor;

        public DataManagerWindow(Editor editor)
        {
            this.editor = editor;

            InitializeComponent();
        }

        void FillInSavedSettingsValues()
        {
            this.TextBox_gameClientPath.Text = Engine.Engine.settings.dataManager?.gameClientPath;
            this.TextBox_assetDatabasePath.Text = Engine.Engine.settings.dataManager?.assetDatabasePath;
        }

        #region Browse Buttons

        // Browse buttons must only change the path, and do nothing else
        private void Button_gameClientPathBrowse_Click(object sender, RoutedEventArgs e)
        {
            var dialog = new System.Windows.Forms.FolderBrowserDialog();
            System.Windows.Forms.DialogResult result = dialog.ShowDialog();
            if (result == System.Windows.Forms.DialogResult.OK)
            {
                TextBox_gameClientPath.Text = dialog.SelectedPath;
                if (Engine.Engine.settings.dataManager != null)
                {
                    Engine.Engine.settings.dataManager.gameClientPath = dialog.SelectedPath;
                    SettingsSerializer.Save();
                }
            }
        }

        private void Button_assetDatabasePathBrowse_Click(object sender, RoutedEventArgs e)
        {
            var dialog = new System.Windows.Forms.OpenFileDialog();
            dialog.FileName = Engine.Data.DataManager.ASSETDB_FILENAME;

            System.Windows.Forms.DialogResult result = dialog.ShowDialog();
            if (result == System.Windows.Forms.DialogResult.OK)
            {
                TextBox_assetDatabasePath.Text = dialog.SafeFileName;
                if (Engine.Engine.settings.dataManager != null)
                {
                    Engine.Engine.settings.dataManager.assetDatabasePath = dialog.SafeFileName;
                    SettingsSerializer.Save();
                }
            }
        }

        #endregion

        #region Asset Database

        public void LogProgress(float progress)
        {
            ProgressBar_Progress.Value = progress;
        }

        public void LogProgressText(string? text)
        {
            Label_Progress.Content = text;
        }

        private void Button_assetDatabasePathCreate_Click(object sender, RoutedEventArgs e)
        {
            if (TextBox_assetDatabasePath.Text != null && TextBox_assetDatabasePath.Text != String.Empty)
            {
                if (System.Windows.MessageBox.Show(this, $"Build new asset database from game data at location? {TextBox_assetDatabasePath.Text}", "Build Asset DB", MessageBoxButton.OKCancel) == MessageBoxResult.OK)
                {
                    Engine.Data.DataManager.CreateAssetDB(TextBox_assetDatabasePath.Text, LogProgress, LogProgressText);
                    return;
                }
            }
            
            var dialog = new System.Windows.Forms.FolderBrowserDialog();
            System.Windows.Forms.DialogResult result = dialog.ShowDialog();
            if (result == System.Windows.Forms.DialogResult.OK)
            {
                string path = Path.Combine(dialog.SelectedPath, Engine.Data.DataManager.ASSETDB_FILENAME);
                TextBox_assetDatabasePath.Text = path;

                if (Engine.Engine.settings.dataManager != null)
                {
                    Engine.Engine.settings.dataManager.assetDatabasePath = path;
                    SettingsSerializer.Save();
                }

                if (System.Windows.MessageBox.Show(this, $"Build new asset database from game data at location? {dialog.SelectedPath}", "Build Asset DB", MessageBoxButton.OKCancel) == MessageBoxResult.OK)
                {
                    Engine.Data.DataManager.CreateAssetDB(path);
                }
            }
        }

        private void Button_assetDatabasePathLoad_Click(object sender, RoutedEventArgs e)
        {
            if (Engine.Engine.settings.dataManager != null)
            {
                if (Engine.Engine.settings.dataManager.assetDatabasePath == null || Engine.Engine.settings.dataManager.assetDatabasePath == String.Empty)
                {
                    System.Windows.MessageBox.Show(this, "Asset database path was empty, Browse or Create a new asset database before loading.", "No Asset DB Path", MessageBoxButton.OK);
                    return;
                }

                if (System.Windows.MessageBox.Show(this, $"Load asset database at location? {Engine.Engine.settings.dataManager.assetDatabasePath}", "Load Asset DB", MessageBoxButton.OKCancel) == MessageBoxResult.OK)
                {
                    Engine.Data.DataManager.LoadAssetDatabase(Engine.Engine.settings.dataManager.assetDatabasePath);
                }
            }
        }

        #endregion

        private void Window_Closing(object sender, System.ComponentModel.CancelEventArgs e)
        {
            //e.Cancel = true;
            //this.Visibility = Visibility.Hidden;
        }

        private void TextBox_gameClientPath_TextChanged(object sender, TextChangedEventArgs e)
        {
            if (Engine.Engine.settings.dataManager != null)
            {
                Engine.Engine.settings.dataManager.gameClientPath = TextBox_gameClientPath.Text;
                SettingsSerializer.Save();
            }
        }

        private void TextBox_assetDatabasePath_TextChanged(object sender, TextChangedEventArgs e)
        {
            if (Engine.Engine.settings.dataManager != null)
            {
                Engine.Engine.settings.dataManager.assetDatabasePath = TextBox_assetDatabasePath.Text;
                SettingsSerializer.Save();
            }
        }

        private void Window_Loaded(object sender, RoutedEventArgs e)
        {
            FillInSavedSettingsValues();
        }

        private void Button_exportAllGameData_Click(object sender, RoutedEventArgs e)
        {
            var dialog = new System.Windows.Forms.FolderBrowserDialog();
            System.Windows.Forms.DialogResult result = dialog.ShowDialog();
            if (result == System.Windows.Forms.DialogResult.OK)
            {
                Engine.Data.DataManager.ExportAllGameData(dialog.SelectedPath, LogProgress, LogProgressText);
            }
        }
    }
}
