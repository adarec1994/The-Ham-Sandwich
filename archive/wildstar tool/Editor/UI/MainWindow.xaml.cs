/************************************************************************
   AvalonDock

   Copyright (C) 2007-2013 Xceed Software Inc.

   This program is provided to you under the terms of the Microsoft Public
   License (Ms-PL) as published at https://opensource.org/licenses/MS-PL
 ************************************************************************/

using AvalonDock;
using AvalonDock.Layout;
using OpenTK.Wpf;
using ProjectWS.Editor.UI;
using ProjectWS.Engine;
using System.Windows;
using System.Windows.Input;

namespace ProjectWS.Editor
{
	/// <summary>
	/// Interaction logic for MainWindow.xaml
	/// </summary>
	public partial class MainWindow : Window
	{
		public Editor editor;

        public MainWindow()
		{
			if (Program.editor == null) return;

			InitializeComponent();
			this.DataContext = this;
			this.editor = Program.editor;
			Program.mainWindow = this;

            Mouse.AddMouseWheelHandler(Application.Current.MainWindow, new MouseWheelEventHandler(this.editor.MouseWheelEventHandler));

            // Create main render/update loop renderer
            this.editor.CreateRendererPane(this, "World", 0, 0, out _);

			if (Engine.Engine.settings.window != null)
			{
				if (Engine.Engine.settings.window.panels.ContainsKey("ToolboxPane"))
					if (Engine.Engine.settings.window.panels["ToolboxPane"].open)
						this.editor.OpenToolboxPane(this);

				if (Engine.Engine.settings.window.panels.ContainsKey("WorldManagerPane"))
					if (Engine.Engine.settings.window.panels["WorldManagerPane"].open)
						this.editor.OpenWorldManagerPane(this);
			}

			this.editor.OnMainWindowCreated();
        }

		#region FocusedElement

		/// <summary>
		/// FocusedElement Dependency Property
		/// </summary>
		public static readonly DependencyProperty FocusedElementProperty =
			DependencyProperty.Register("FocusedElement", typeof(string), typeof(MainWindow),
				new FrameworkPropertyMetadata((IInputElement)null));

		/// <summary>
		/// Gets or sets the FocusedElement property.  This dependency property 
		/// indicates ....
		/// </summary>
		public string FocusedElement
		{
			get => (string)GetValue(FocusedElementProperty);
			set => SetValue(FocusedElementProperty, value);
		}

        #endregion

        private void OnLayoutRootPropertyChanged(object sender, System.ComponentModel.PropertyChangedEventArgs e)
        {
            var activeContent = ((LayoutRoot)sender).ActiveContent;
            if (e.PropertyName == "ActiveContent")
            {
				if (activeContent == null)
					return;

				GLWpfControl? focusedControl = null;

                if (activeContent.Content is WorldRendererPane)
				{
					var worldRendererPane = activeContent.Content as WorldRendererPane;
					if (worldRendererPane != null)
						focusedControl = worldRendererPane.GetOpenTKControl();
                }
                if (activeContent.Content is ModelRendererPane)
                {
                    var modelRendererPane = activeContent.Content as ModelRendererPane;
                    if (modelRendererPane != null)
                        focusedControl = modelRendererPane.GetOpenTKControl();
                }
                if (activeContent.Content is WorldManagerPane)
                {
                    var modelRendererPane = activeContent.Content as WorldManagerPane;
                    if (modelRendererPane != null)
                        focusedControl = modelRendererPane.GetOpenTKControl();
                }

                if (focusedControl != null && this.editor != null)
				{
					this.editor.focusedControl = focusedControl;

                    foreach (var item in this.editor.controls)
					{
						if (item.Value == focusedControl)
						{
							Debug.Log(string.Format("Active Control -> {0}", item.Key));
							if (this.editor != null && this.editor.engine != null)
								this.editor.engine.focusedRendererID = item.Key;
						}
					}

					return;
				}
            }
        }

        private void DockManager_DocumentClosing(object sender, DocumentClosingEventArgs e)
		{
			if (MessageBox.Show("Are you sure you want to close the document?", "AvalonDock Sample", MessageBoxButton.YesNo) == MessageBoxResult.No)
				e.Cancel = true;
		}

		private void Window_Loaded(object sender, RoutedEventArgs e)
		{
			this.Top = Engine.Engine.settings.window.top;
            this.Left = Engine.Engine.settings.window.left;
            this.Width = Engine.Engine.settings.window.width;
            this.Height = Engine.Engine.settings.window.height;

			this.WindowState = (WindowState)Engine.Engine.settings.window.windowState;
        }

		private void Window_Closing(object sender, System.ComponentModel.CancelEventArgs e)
		{
			Engine.Engine.settings.window.top = this.Top;
            Engine.Engine.settings.window.left = this.Left;
            Engine.Engine.settings.window.width = this.Width;
            Engine.Engine.settings.window.height = this.Height;

            Engine.Engine.settings.window.windowState = (int)this.WindowState;

            SettingsSerializer.Save();

			this.editor.OnExit();
        }

		private void menuItem_Toolbox_Click(object sender, RoutedEventArgs e)
		{
			this.editor.OpenToolboxPane(this);
        }

		private void menuItem_DataManager_Click(object sender, RoutedEventArgs e)
		{
			this.editor.OpenDataManager();
        }

		private void menuItm_newProject_Click(object sender, RoutedEventArgs e)
		{
			this.editor.NewProject();
		}

		private void menuItm_save_Click(object sender, RoutedEventArgs e)
		{
            this.editor.Save();
        }

		private void menuItm_loadProject_Click(object sender, RoutedEventArgs e)
		{
			this.editor.LoadProject();
		}

		private void button_WorldManager_Click(object sender, RoutedEventArgs e)
		{
			this.editor.OpenWorldManagerPane(this);
		}

		private void menuItem_WorldManager_Click(object sender, RoutedEventArgs e)
		{
            this.editor.OpenWorldManagerPane(this);
        }
	}
}