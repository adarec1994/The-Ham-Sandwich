using AvalonDock.Layout;
using MathUtils;
using OpenTK.Wpf;
using ProjectWS.Editor.Project;
using ProjectWS.Editor.Tools;
using ProjectWS.Editor.UI;
using ProjectWS.Engine;
using ProjectWS.Engine.Data;
using ProjectWS.Engine.Project;
using ProjectWS.Engine.Rendering;
using ProjectWS.Engine.World;
using System;
using System.Collections.Generic;
using System.IO;
using System.IO.Pipes;
using System.Security.Principal;
using System.Text.Json;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Input;
using System.Xml.Linq;

namespace ProjectWS.Editor
{
    public class Editor
    {
        DateTime time1 = DateTime.Now;
        DateTime time2 = DateTime.Now;
        readonly float timeScale = 1.0f;
        float deltaTime;
        float elapsedTime;
        public float mouseWheelPos;
        public bool keyboardFocused;

        public Engine.Engine? engine;
        private TestArea.Tests? tests;
        public GLWpfControl? focusedControl;
        public Dictionary<int, GLWpfControl>? controls;
        public Dictionary<int, WorldRendererPane>? worldRendererPanes;
        public Dictionary<int, ModelRendererPane>? modelRendererPanes;
        FPSCounter? fps;
        //public GLWpfControl? mapRendererControl;

        // Common Layout
        public LayoutAnchorablePane layoutAnchorablePane;
        public LayoutDocumentPane layoutDocumentPane;

        // Sky Editor
        public SkyEditorPane? skyEditorPane;

        // Toolbox
        public List<Tool>? tools;
        public UI.Toolbox.ToolboxPane? toolboxPane;
        public LayoutAnchorable toolboxLayoutAnchorable;

        // World Manager
        public WorldManagerPane? worldManagerPane;
        public MapRenderer? mapRenderer;
        public LayoutDocument worldManagerDocument;

        // Data Manager
        public DataManagerWindow dataManagerWindow;

        // Map Editor
        public MapEditorWindow mapEditorWindow;

        // Map Import
        public MapImportWindow mapImportWindow;


        Dictionary<OpenTK.Windowing.GraphicsLibraryFramework.Keys, Engine.Input.User32Wrapper.Key> opentkKeyMap = new Dictionary<OpenTK.Windowing.GraphicsLibraryFramework.Keys, Engine.Input.User32Wrapper.Key>()
        {
            { OpenTK.Windowing.GraphicsLibraryFramework.Keys.W, Engine.Input.User32Wrapper.Key.W },
            { OpenTK.Windowing.GraphicsLibraryFramework.Keys.S, Engine.Input.User32Wrapper.Key.S },
            { OpenTK.Windowing.GraphicsLibraryFramework.Keys.A, Engine.Input.User32Wrapper.Key.A },
            { OpenTK.Windowing.GraphicsLibraryFramework.Keys.D, Engine.Input.User32Wrapper.Key.D },
            { OpenTK.Windowing.GraphicsLibraryFramework.Keys.C, Engine.Input.User32Wrapper.Key.C },
            { OpenTK.Windowing.GraphicsLibraryFramework.Keys.Space, Engine.Input.User32Wrapper.Key.Space },
            { OpenTK.Windowing.GraphicsLibraryFramework.Keys.R, Engine.Input.User32Wrapper.Key.R },
            { OpenTK.Windowing.GraphicsLibraryFramework.Keys.LeftAlt, Engine.Input.User32Wrapper.Key.Alt },
            { OpenTK.Windowing.GraphicsLibraryFramework.Keys.RightAlt, Engine.Input.User32Wrapper.Key.Alt },
        };

        public Editor()
        {
            this.controls = new Dictionary<int, GLWpfControl>();
            this.worldRendererPanes = new Dictionary<int, WorldRendererPane>();
            this.modelRendererPanes = new Dictionary<int, ModelRendererPane>();
            //this.mapRendererPanes = new Dictionary<int, WorldManagerPane>();
            this.tools = new List<Tool>();
            Start();
        }

        void Start()
        {
            // Create Engine
            this.engine = new Engine.Engine();

            // Initialize Input
            InputInitialize();

            // Load asset database (if it exists)
            Engine.Data.DataManager.LoadAssetDatabase(Engine.Engine.settings.dataManager.assetDatabasePath);

            // Auto load previous open project on startup
            Engine.Project.ProjectManager.LoadProject(Engine.Engine.settings.projectManager.previousLoadedProject);

            // Create tests
            this.tests = new TestArea.Tests(this.engine, this);

            // Create framerate counter
            this.fps = new FPSCounter();
        }

        public void OnMainWindowCreated()
        {
            // Run tests
            this.tests.Start();
        }

        public void Update()
        {   
            CalculateDeltaTime();
            elapsedTime += deltaTime;
            if (this.engine != null)
            {
                InputUpdate();
                this.engine.Update(this.deltaTime, this.timeScale);

                for (int i = 0; i < this.tools?.Count; i++)
                {
                    if (this.tools[i].isEnabled)
                        this.tools[i].Update(this.deltaTime);
                }

                if (Program.app != null && this.fps != null)
                    Program.app.MainWindow.Title = $"FPS:{this.fps.Get()} DrawCalls:{WorldRenderer.drawCalls} PropDrawCalls:{WorldRenderer.propDrawCalls} VRam Usage:{this.engine.memory_usage_mb}MB/{this.engine.total_mem_mb}MB";
            }
        }

        public void Render(int renderer, int frameBuffer)
        {
            if (this.engine != null)
                this.engine.Render(renderer, frameBuffer);

            if (this.fps != null)
                this.fps.Update(this.deltaTime);
        }

        void CalculateDeltaTime()
        {
            this.time2 = DateTime.Now;
            this.deltaTime = (this.time2.Ticks - this.time1.Ticks) / 10000000f;
            this.time1 = this.time2;
        }

        void InputInitialize()
        {
            if (this.engine == null) return;

            foreach (var item in opentkKeyMap)
            {
                this.engine.input.keyStates[item.Key] = false;
            }
        }

        void InputUpdate()
        {
            if (this.engine == null) return;

            this.keyboardFocused = Application.Current.MainWindow.IsKeyboardFocusWithin;

            if (this.keyboardFocused)
            {
                foreach (var item in opentkKeyMap)
                {
                    //this.engine.input.keyStates[item.Key] = Keyboard.IsKeyDown(item.Value);
                    this.engine.input.keyStates[item.Key] = Engine.Input.User32Wrapper.GetAsyncKeyState(item.Value) != 0;
                }
            }

            //var mousePosition = Mouse.GetPosition(this.focusedControl);

            ProjectWS.Engine.Input.User32Wrapper.GetCursorPos(out var lpPoint);
            var mousePosition = this.focusedControl.PointFromScreen(new Point(lpPoint.X, lpPoint.Y));

            this.engine.input.mousePosPerControl[this.engine.focusedRendererID] = new Vector3((float)mousePosition.X, (float)mousePosition.Y, this.mouseWheelPos);
            this.mouseWheelPos = 0;
            //for (int r = 0; r < this.engine.renderers.Count; r++)
            foreach(var rendererItem in this.engine.renderers)
            {
                //var renderer = this.engine.renderers[r];
                var renderer = rendererItem.Value;
                renderer.RecalculateViewports();
                if (renderer.ID == engine.focusedRendererID)
                {
                    if(this.worldRendererPanes.TryGetValue(renderer.ID, out WorldRendererPane? wPane))
                    {
                        if (renderer.viewports != null)
                        {
                            if ((Mouse.LeftButton == MouseButtonState.Pressed && !this.engine.input.LMB) || (Mouse.RightButton == MouseButtonState.Pressed && !this.engine.input.RMB))
                            {
                                if (renderer.viewports.Count == 1)
                                {
                                    renderer.viewports[0].interactive = true;
                                }
                                else
                                {
                                    for (int v = 0; v < renderer.viewports.Count; v++)
                                    {
                                        var vp = renderer.viewports[v];
                                        if (mousePosition.X < vp.x + vp.width && mousePosition.X > vp.x &&
                                            mousePosition.Y < vp.y + vp.height && mousePosition.Y > vp.y)
                                        {
                                            vp.interactive = true;
                                        }
                                        else
                                        {
                                            vp.interactive = false;
                                        }
                                    }
                                }
                            }

                            if (renderer.viewports.Count > 1)
                            {
                                wPane.ViewportRect0.Visibility = Visibility.Visible;

                                for (int v = 0; v < renderer.viewports.Count; v++)
                                {
                                    if (renderer.viewports[v].interactive)
                                    {
                                        wPane.ViewportRect0.Margin = new Thickness(renderer.viewports[v].x, renderer.viewports[v].y, 0, 0);
                                        wPane.ViewportRect0.Width = renderer.viewports[v].width;
                                        if (renderer.viewports[v].height > 0)
                                            wPane.ViewportRect0.Height = renderer.viewports[v].height - 2;
                                    }
                                }
                            }
                            else
                            {
                                wPane.ViewportRect0.Visibility = Visibility.Hidden;
                            }
                        }
                    }

                    if (this.modelRendererPanes.TryGetValue(renderer.ID, out ModelRendererPane? mPane))
                    {
                        if (renderer.viewports != null)
                        {
                            if ((Mouse.LeftButton == MouseButtonState.Pressed && !this.engine.input.LMB) || (Mouse.RightButton == MouseButtonState.Pressed && !this.engine.input.RMB))
                            {
                                if (renderer.viewports.Count == 1)
                                {
                                    renderer.viewports[0].interactive = true;
                                }
                                else
                                {
                                    for (int v = 0; v < renderer.viewports.Count; v++)
                                    {
                                        var vp = renderer.viewports[v];
                                        if (mousePosition.X < vp.x + vp.width && mousePosition.X > vp.x &&
                                            mousePosition.Y < vp.y + vp.height && mousePosition.Y > vp.y)
                                        {
                                            vp.interactive = true;
                                        }
                                        else
                                        {
                                            vp.interactive = false;
                                        }
                                    }
                                }
                            }

                            if (renderer.viewports.Count > 1)
                            {
                                mPane.ViewportRect0.Visibility = Visibility.Visible;

                                for (int v = 0; v < renderer.viewports.Count; v++)
                                {
                                    if (renderer.viewports[v].interactive)
                                    {
                                        mPane.ViewportRect0.Margin = new Thickness(renderer.viewports[v].x, renderer.viewports[v].y, 0, 0);
                                        mPane.ViewportRect0.Width = renderer.viewports[v].width;
                                        if (renderer.viewports[v].height > 0)
                                            mPane.ViewportRect0.Height = renderer.viewports[v].height - 2;
                                    }
                                }
                            }
                            else
                            {
                                mPane.ViewportRect0.Visibility = Visibility.Hidden;
                            }
                        }
                    }
                }
            }

            this.engine.input.LMB = Engine.Input.User32Wrapper.GetAsyncKeyState(Engine.Input.User32Wrapper.Key.LeftMouseBtn) != 0;
            this.engine.input.RMB = Engine.Input.User32Wrapper.GetAsyncKeyState(Engine.Input.User32Wrapper.Key.RightMouseBtn) != 0;
            this.engine.input.MMB = Engine.Input.User32Wrapper.GetAsyncKeyState(Engine.Input.User32Wrapper.Key.MidMouseBtn) != 0;
            //this.engine.input.LMB = Mouse.LeftButton == MouseButtonState.Pressed;
            //this.engine.input.RMB = Mouse.RightButton == MouseButtonState.Pressed;
            //this.engine.input.MMB = Mouse.MiddleButton == MouseButtonState.Pressed;

            // Allow mouse drag beyond the window borders
            if (this.engine.input.RMB)
                Application.Current.MainWindow.CaptureMouse();
            else
                Application.Current.MainWindow.ReleaseMouseCapture();
        }

        public void MouseWheelEventHandler(object sender, MouseWheelEventArgs e)
        {
            this.mouseWheelPos += e.Delta / 120f;
        }

        public Renderer? CreateRendererPane(MainWindow window, string name, int ID, int type, out LayoutDocumentPane testRenderPane)
        {
            testRenderPane = null;

            if (this.engine == null) return null;

            Debug.Log("Create Renderer Pane, type " + type);

            Renderer renderer;
            GLWpfControl openTkControl;
            Grid rendererGrid;

            if (type == 0)
            {
                var rendererPane = new WorldRendererPane(this);
                openTkControl = rendererPane.GetOpenTKControl();
                rendererGrid = rendererPane.GetRendererGrid();
                this.controls?.Add(ID, openTkControl);

                var layoutDoc = new LayoutDocument();
                layoutDoc.Title = name;
                layoutDoc.ContentId = "Renderer_" + ID + "_" + name;
                layoutDoc.Content = rendererPane;
                layoutDoc.CanClose = false;

                testRenderPane = new LayoutDocumentPane(layoutDoc);

                window.LayoutDocumentPaneGroup.Children.Add(testRenderPane);

                var settings = new GLWpfControlSettings { MajorVersion = 4, MinorVersion = 0, RenderContinuously = true };
                openTkControl.Start(settings);

                var worldRenderer = new WorldRenderer(this.engine, ID, this.engine.input);
                renderer = worldRenderer;

                // Create tools
                this.tools?.Add(new TerrainSculptTool(this.engine, this, worldRenderer));
                this.tools?.Add(new TerrainLayerPaintTool(this.engine, this, worldRenderer));
                this.tools?.Add(new PropTool(this.engine, this, worldRenderer));

                this.engine.renderers.Add(ID, renderer);
                /*
                var gizmo = new Engine.Objects.Gizmos.BoxGizmo(Vector4.One);
                gizmo.transform.SetPosition(0.1f, 0.1f, 0.1f);
                if (renderer.gizmos != null)
                    renderer.gizmos.Add(gizmo);

                this.engine.taskManager.buildTasks.Enqueue(new Engine.TaskManager.BuildObjectTask(gizmo));
                */
                rendererPane.changeViewMode = renderer.SetViewportMode;
                rendererPane.toggleFog = renderer.ToggleFog;
                rendererPane.toggleAreaGrid = renderer.ToggleAreaGrid;
                rendererPane.toggleChunkGrid = renderer.ToggleChunkGrid;
                rendererPane.fogToggle.IsChecked = Engine.Engine.settings.wRenderer.toggles.fog;
                rendererPane.displayAreaToggle.IsChecked = Engine.Engine.settings.wRenderer.toggles.displayAreaGrid;
                rendererPane.displayChunkToggle.IsChecked = Engine.Engine.settings.wRenderer.toggles.displayChunkGrid;

                this.worldRendererPanes?.Add(ID, rendererPane);
            }
            else if (type == 1)
            {
                var rendererPane = new ModelRendererPane();
                openTkControl = rendererPane.GetOpenTKControl();
                rendererGrid = rendererPane.GetRendererGrid();
                this.controls?.Add(ID, openTkControl);

                var layoutDoc = new LayoutDocument();
                layoutDoc.Title = name;
                layoutDoc.ContentId = "Renderer_" + ID + "_" + name;
                layoutDoc.Content = rendererPane;

                testRenderPane = new LayoutDocumentPane(layoutDoc);

                window.LayoutDocumentPaneGroup.Children.Add(testRenderPane);

                var settings = new GLWpfControlSettings { MajorVersion = 4, MinorVersion = 0, RenderContinuously = true };
                openTkControl.Start(settings);

                layoutDoc.DockAsDocument(); // This makes it [World][Model] tab

                renderer = new ModelRenderer(this.engine, ID, this.engine.input);
                //renderer.SetDimensions(0, 0, (int)openTkControl.ActualWidth, (int)openTkControl.ActualHeight);
                this.engine.renderers.Add(ID, renderer);

                rendererPane.changeRenderMode = renderer.SetShadingOverride;
                this.modelRendererPanes?.Add(ID, rendererPane);
            }
                /*
            else if (type == 2)
            {
                renderer = new MapRenderer(this.engine, ID, this.engine.input);

                var rendererPane = new WorldManagerPane(this, renderer as MapRenderer);
                openTkControl = rendererPane.GetOpenTKControl();
                rendererGrid = rendererPane.GetRendererGrid();
                this.controls?.Add(ID, openTkControl);

                var layoutDoc = new LayoutDocument();
                layoutDoc.Title = name;
                layoutDoc.ContentId = "Renderer_" + ID + "_" + name;
                layoutDoc.Content = rendererPane;

                testRenderPane = new LayoutDocumentPane(layoutDoc);

                window.LayoutDocumentPaneGroup.Children.Add(testRenderPane);

                var settings = new GLWpfControlSettings { MajorVersion = 4, MinorVersion = 0, RenderContinuously = true };
                openTkControl.Start(settings);

                //renderer.SetDimensions(0, 0, (int)openTkControl.ActualWidth, (int)openTkControl.ActualHeight);
                this.engine.renderers.Add(renderer);

                this.mapRendererPaneID = ID;
                this.mapRendererPanes?.Add(ID,rendererPane);
            }
                */
            else
            {
                Debug.Log("Unsupported renderer type : " + type);
                return null;
            }

            // Add events
            openTkControl.Render += (delta) => OpenTkControl_OnRender(delta, ID, openTkControl.Framebuffer);
            openTkControl.Loaded += (sender, e) => OpenTkControl_OnLoaded(sender, e, renderer, rendererGrid, openTkControl);
            openTkControl.SizeChanged += (sender, e) => OpenTkControl_OnSizeChanged(sender, e, renderer);
            
            return renderer;
        }

        public void OpenToolboxPane(MainWindow window)
        {
            if (this.toolboxLayoutAnchorable != null)
            {
                if (!this.layoutAnchorablePane.Children.Contains(this.toolboxLayoutAnchorable))
                    this.layoutAnchorablePane.Children.Add(this.toolboxLayoutAnchorable);

                return;
            }

            if (this.toolboxPane == null)
            {
                this.toolboxPane = new UI.Toolbox.ToolboxPane();
                this.toolboxPane.engine = this.engine;
                this.toolboxPane.editor = this;
            }

            if (this.toolboxLayoutAnchorable == null)
            {
                this.toolboxLayoutAnchorable = new LayoutAnchorable();
                this.toolboxLayoutAnchorable.Title = "Toolbox";
                this.toolboxLayoutAnchorable.ContentId = "ToolboxPane";
                this.toolboxLayoutAnchorable.Content = this.toolboxPane;
                this.toolboxLayoutAnchorable.PropertyChanged += LayoutAnchorable_PropertyChanged;
            }

            if (this.layoutAnchorablePane == null)
            {
                this.layoutAnchorablePane = new LayoutAnchorablePane(this.toolboxLayoutAnchorable);
                window.LayoutAnchorablePaneGroup.Children.Add(this.layoutAnchorablePane);
            }

            for (int i = 0; i < this.tools?.Count; i++)
            {
                if (this.tools[i] is PropTool)
                {
                    this.tools[i].OnTooboxPaneLoaded();
                }
            }
        }

        public void OpenWorldManagerPane(MainWindow window)
        {
            if (this.worldManagerDocument != null)
            {
                if (!this.layoutDocumentPane.Children.Contains(this.worldManagerDocument))
                    this.layoutDocumentPane.Children.Add(this.worldManagerDocument);

                return;
            }

            int ID = 1000;
            if (this.mapRenderer == null)
            {
                this.mapRenderer = new MapRenderer(this.engine, ID, this.engine.input);
                this.engine.renderers.Add(ID, this.mapRenderer);
            }

            if (this.worldManagerPane == null)
            {
                this.worldManagerPane = new WorldManagerPane(this, this.mapRenderer);

                var openTkControl = this.worldManagerPane.GetOpenTKControl();
                var rendererGrid = this.worldManagerPane.GetRendererGrid();
                Engine.Rendering.Renderer renderer = this.mapRenderer;

                var settings = new GLWpfControlSettings { MajorVersion = 4, MinorVersion = 0, RenderContinuously = true };
                openTkControl.Start(settings);

                this.controls?.Add(ID, openTkControl);

                // Add events
                openTkControl.Render += (delta) => OpenTkControl_OnRender(delta, ID, openTkControl.Framebuffer);
                openTkControl.Loaded += (sender, e) => OpenTkControl_OnLoaded(sender, e, renderer, rendererGrid, openTkControl);
                openTkControl.SizeChanged += (sender, e) => OpenTkControl_OnSizeChanged(sender, e, renderer);
            }

            if (this.worldManagerDocument == null)
            {
                this.worldManagerDocument = new LayoutDocument();
                this.worldManagerDocument.Title = "World Manager";
                this.worldManagerDocument.ContentId = "WorldManagerPane";
                this.worldManagerDocument.Content = this.worldManagerPane;

                this.worldManagerDocument.PropertyChanged += LayoutDocument_PropertyChanged;
            }

            if (this.layoutDocumentPane == null)
            {
                this.layoutDocumentPane = new LayoutDocumentPane(this.worldManagerDocument);
                window.LayoutDocumentPaneGroup.Children.Add(this.layoutDocumentPane);
            }
        }

        void OpenTkControl_OnRender(TimeSpan delta, int ID, int frameBuffer)
        {
            // Find which control is focused, and only update if ID matches
            // This is so that Update is only called one time, not for every render control call
            if (ID == this.engine?.focusedRendererID)
            {
                Update();
            }

            Render(ID, frameBuffer);
        }

        void OpenTkControl_OnLoaded(object sender, RoutedEventArgs e, Renderer renderer, Grid control, GLWpfControl openTkControl)
        {
            renderer.SetDimensions(0, 0, (int)openTkControl.ActualWidth, (int)openTkControl.ActualHeight);
            renderer.Load();
        }

        void OpenTkControl_OnSizeChanged(object sender, SizeChangedEventArgs e, Renderer renderer)
        {
            var size = e.NewSize;
            renderer.Resize((int)size.Width, (int)size.Height);
        }

        internal void Save()
        {
            ProjectManager.SaveProject();

            /*
            foreach (var wItem in this.engine?.worlds)
            {
                foreach (var cItem in wItem.Value.chunks)
                {
                    cItem.Value.area.ProcessForExport();
                    cItem.Value.area.Write();
                }
            }

            // Update sandbox
            this.pipeClient = new NamedPipeClientStream(".", "Arctium.ChatCommand.Pipe", PipeDirection.InOut, PipeOptions.WriteThrough | PipeOptions.Asynchronous, TokenImpersonationLevel.Impersonation);
            this.pipeClient.Connect();

            using (var bw = new BinaryWriter(this.pipeClient))
            {
                var data = System.Text.Encoding.ASCII.GetBytes("!tele 256 -998 256 3538");
                bw.Write(data.Length);
                bw.Write(data);
            }
            */
            //pipeClient.Close();
        }

        internal void SandboxTeleport(float x, float y, float z, uint mapID)
        {
            var pipeClient = new NamedPipeClientStream(".", "Arctium.ChatCommand.Pipe", PipeDirection.InOut, PipeOptions.WriteThrough | PipeOptions.Asynchronous, TokenImpersonationLevel.Impersonation);
            pipeClient.Connect();

            using (var bw = new BinaryWriter(pipeClient))
            {
                var data = System.Text.Encoding.ASCII.GetBytes($"!tele {x} {y} {z} {mapID}");
                bw.Write(data.Length);
                bw.Write(data);
            }
        }

        internal void EngineTeleport(float x, float y, float z)
        {
            this.engine?.world?.Teleport(x, y, z);
        }

        internal void OpenDataManager()
        {
            this.dataManagerWindow = new DataManagerWindow(this);
            this.dataManagerWindow.Owner = Program.mainWindow;
            this.dataManagerWindow.Show();
        }

        internal void LoadProject()
        {
            var dialog = new System.Windows.Forms.OpenFileDialog();
            dialog.DefaultExt = "wsProject";
            dialog.Filter = $"Project Files (*.{ProjectManager.PROJECT_EXTENSION})|*.{ProjectManager.PROJECT_EXTENSION}";

            System.Windows.Forms.DialogResult result = dialog.ShowDialog();
            if (result == System.Windows.Forms.DialogResult.OK)
            {
                ProjectManager.LoadProject(Path.GetFullPath(dialog.FileName));
            }
        }

        internal void NewProject()
        {
            var dialog = new System.Windows.Forms.SaveFileDialog();
            dialog.DefaultExt = ProjectManager.PROJECT_EXTENSION;
            dialog.Filter = $"Project Files (*.{ProjectManager.PROJECT_EXTENSION})|*.{ProjectManager.PROJECT_EXTENSION}";

            System.Windows.Forms.DialogResult result = dialog.ShowDialog();
            if (result == System.Windows.Forms.DialogResult.OK)
            {
                ProjectManager.CreateProject(Path.GetFullPath(dialog.FileName));
            }
        }

        private void LayoutAnchorable_PropertyChanged(object? sender, System.ComponentModel.PropertyChangedEventArgs e)
        {
            if (e.PropertyName == "Parent")
            {
                var layoutAnchor = sender as LayoutAnchorable;
                if (layoutAnchor != null)
                {
                    if (!Engine.Engine.settings.window!.panels.ContainsKey(layoutAnchor.ContentId))
                        Engine.Engine.settings.window.panels.Add(layoutAnchor.ContentId, new Settings.Window.Panel());

                    Engine.Engine.settings.window.panels[layoutAnchor.ContentId].open = layoutAnchor.Parent != null;
                    Engine.SettingsSerializer.Save();
                }
            }
        }

        private void LayoutDocument_PropertyChanged(object? sender, System.ComponentModel.PropertyChangedEventArgs e)
        {
            if(e.PropertyName == "Parent")
            {
                var layoutDoc = sender as LayoutDocument;
                if (layoutDoc != null)
                {
                    if (!Engine.Engine.settings.window!.panels.ContainsKey(layoutDoc.ContentId))
                        Engine.Engine.settings.window.panels.Add(layoutDoc.ContentId, new Settings.Window.Panel());

                    Engine.Engine.settings.window.panels[layoutDoc.ContentId].open = layoutDoc.Parent != null;
                    Engine.SettingsSerializer.Save();
                }
            }
        }

        internal void OpenCreateMapWindow()
        {
            this.mapEditorWindow = new MapEditorWindow(this, true);
            this.mapEditorWindow.Owner = Program.mainWindow;
            this.mapEditorWindow.Show();
        }

        internal void EditMap()
        {
            int index = this.worldManagerPane.mapComboBox.SelectedIndex;
            if (index == -1)
                return;

            this.mapEditorWindow = new MapEditorWindow(this, false);
            this.mapEditorWindow.Owner = Program.mainWindow;
            this.mapEditorWindow.Show();

            if (ProjectManager.project?.Maps != null && ProjectManager.project.Maps.Count > index && index >= 0)
            {
                var map = ProjectManager.project.Maps[index];

                this.mapEditorWindow.FillInputs(map);
            }
        }

        internal void OpenImportMapWindow()
        {
            this.mapImportWindow = new MapImportWindow(this);
            this.mapImportWindow.Owner = Program.mainWindow;
            this.mapImportWindow.Show();
        }

        internal void RemoveMap()
        {
            if (System.Windows.MessageBox.Show("Are you sure you want to remove this map and all of the assets that belong to it?", "Remove map ?", MessageBoxButton.OKCancel) == MessageBoxResult.OK)
            {
                // Remove from dropdown
                if (this.worldManagerPane == null) return;
                if (ProjectManager.project == null) return;
                if (ProjectManager.project.Maps == null) return;

                int idx = this.worldManagerPane.mapComboBox.SelectedIndex;
                if (idx == -1) return;

                var ID = this.worldManagerPane.mapIDs[idx];
                //var name = mapRendererPane.mapComboBox.Items[idx].ToString();
                this.worldManagerPane.mapIDs.RemoveAt(idx);
                this.worldManagerPane.mapNames.RemoveAt(idx);
                //mapRendererPane.mapComboBox.Items.RemoveAt(idx);

                // Remove from project
                int projIdx = -1;

                for (int i = 0; i < ProjectManager.project.Maps.Count; i++)
                {
                    var map = ProjectManager.project.Maps[i];
                    if (map == null) continue;
                    if (map.worldRecord == null) continue;

                    if (map.worldRecord!.ID == ID)
                    {
                        projIdx = i;
                        break;
                    }
                }

                if (projIdx != -1)
                {
                    // TODO : If the map is loaded in world renderer, unload it

                    // Destroy map assets
                    var map = ProjectManager.project.Maps[projIdx];
                    var gameDir = $"{Archive.rootBlockName}\\{map.worldRecord.assetPath}";
                    var gameDirNoRoot = gameDir.Substring(gameDir.IndexOf('\\'));
                    var projectFolder = $"{Path.GetDirectoryName(ProjectManager.projectFile)}\\{Path.GetFileNameWithoutExtension(ProjectManager.projectFile)}";
                    var realDir = $"{projectFolder}\\{gameDirNoRoot}";

                    if (Directory.Exists(realDir))
                        DeleteDirectory(realDir);

                    ProjectManager.project.Maps.RemoveAt(projIdx);
                    ProjectManager.SaveProject();
                }
            }
        }

        void DeleteDirectory(string target_dir)
        {
            string[] files = Directory.GetFiles(target_dir);
            string[] dirs = Directory.GetDirectories(target_dir);

            foreach (string file in files)
            {
                File.SetAttributes(file, FileAttributes.Normal);
                File.Delete(file);
            }

            foreach (string dir in dirs)
            {
                DeleteDirectory(dir);
            }

            Directory.Delete(target_dir, false);
        }

        internal void ImportGameMap(Engine.Database.Definitions.World worldRecord)
        {
            var mapName = Path.GetFileNameWithoutExtension(worldRecord.assetPath);
            var mapID = worldRecord.ID;
            // Check if map ID is already in use
            for (int i = 0; i < ProjectManager.project?.Maps?.Count; i++)
            {
                if (ProjectManager.project.Maps[i]?.worldRecord?.ID == mapID)
                {
                    // Map already loaded
                    if (System.Windows.MessageBox.Show($"Map ID {mapID} is already in use, continue importing and generate a new ID ?", "Duplicate ID", MessageBoxButton.OKCancel) == MessageBoxResult.OK)
                    {
                        mapID = ++ProjectManager.project.lastWorldID;
                    }
                    else
                    {
                        return;
                    }
                }
            }

            // First create map entry in project, and add to dropdown, and only then fill in the details
            var newMap = new Engine.Project.Project.Map();
            newMap.Name = mapName;
            newMap.isGameMap = true;
            newMap.worldRecord = new Engine.Project.Project.Map.World()
            {
                ID = mapID,
                assetPath = worldRecord.assetPath,
                chunkBounds00 = worldRecord.chunkBounds00,
                chunkBounds01 = worldRecord.chunkBounds01,
                chunkBounds02 = worldRecord.chunkBounds02,
                chunkBounds03 = worldRecord.chunkBounds03,
                flags = worldRecord.flags,
                heroismMenaceLevel = worldRecord.heroismMenaceLevel,
                localizedTextIdName = worldRecord.localizedTextIdName,
                maxItemLevel = worldRecord.maxItemLevel,
                minItemLevel = worldRecord.minItemLevel,
                plugAverageHeight = worldRecord.plugAverageHeight,
                primeLevelMax = worldRecord.primeLevelMax,
                primeLevelOffset = worldRecord.primeLevelOffset,
                rewardRotationContentId = worldRecord.rewardRotationContentId,
                screenModelPath = worldRecord.screenModelPath,
                screenPath = worldRecord.screenPath,
                type = worldRecord.type,
                veteranTierScalingType = worldRecord.veteranTierScalingType
            };

            // Import WorldLocation2 entries into project.map struct
            /*
            newMap.worldLocations = new List<Map.WorldLocation>();
            foreach (var item in DataManager.database.worldLocation.records)
            {
                if (item.Value.worldId == mapID)
                {
                    newMap.worldLocations.Add(new Map.WorldLocation()
                    {
                        ID = item.Key,
                        radius = item.Value.radius,
                        maxVerticalDistance = item.Value.maxVerticalDistance,
                        position0 = item.Value.position0,
                        position1 = item.Value.position1,
                        position2 = item.Value.position2,
                        facing0 = item.Value.facing0,
                        facing1 = item.Value.facing1,
                        facing2 = item.Value.facing2,
                        facing3 = item.Value.facing3,
                        worldId = item.Value.worldId,
                        worldZoneId = item.Value.worldZoneId,
                        phases = item.Value.phases
                    });
                }
            }
            */


            // Add map name to dropdown
            this.worldManagerPane.mapIDs?.Add(mapID);
            this.worldManagerPane.mapNames?.Add($"{mapID}. {mapName}");
            //mapRendererPane.mapComboBox.Items.Add(mapName);

            // Load Game Data
            var gameData = new GameData(this.engine!, Engine.Engine.settings.dataManager?.gameClientPath);
            gameData.Read(false);

            // Extract map assets into project folder and generate chunkInfo.json
            var gameDir = $"{Archive.rootBlockName}\\{worldRecord.assetPath}";
            var fileEntries = gameData.GetFileEntries(gameDir);
            var gameDirNoRoot = gameDir.Substring(gameDir.IndexOf('\\'));
            var projectFolder = $"{Path.GetDirectoryName(ProjectManager.projectFile)}\\{Path.GetFileNameWithoutExtension(ProjectManager.projectFile)}";
            var realDir = $"{projectFolder}\\{gameDirNoRoot}";

            if (fileEntries != null)
            {
                if (!Directory.Exists(realDir))
                    Directory.CreateDirectory(realDir);

                var chunkInfo = new MapChunkInfo();

                newMap.mapChunkInfoPath = $"{realDir}\\ChunkInfo.json";
                chunkInfo.chunks = new List<Vector2i>();
                chunkInfo.chunksLow = new List<Vector2i>();
                chunkInfo.minimaps = new List<Vector2i>();

                const string TEX = ".tex";
                const string AREA = ".area";
                const string LOW = "_Low";

                foreach (var entry in fileEntries)
                {
                    var realFilePath = Path.Combine(realDir, entry.Key);

                    var tokens = entry.Key.Split('.');

                    if (tokens.Length == 3)
                    {
                        var hexToken = tokens[1];
                        var xHex = $"{hexToken[2]}{hexToken[3]}";
                        var yHex = $"{hexToken[0]}{hexToken[1]}";
                        int xValue = int.Parse(xHex, System.Globalization.NumberStyles.HexNumber);
                        int yValue = int.Parse(yHex, System.Globalization.NumberStyles.HexNumber);

                        if (entry.Key.EndsWith(TEX))
                        {
                            chunkInfo.minimaps.Add(new Vector2i(xValue, yValue));
                        }
                        else if (entry.Key.EndsWith(AREA))
                        {
                            if (entry.Key.Contains(LOW))
                            {
                                chunkInfo.chunksLow.Add(new Vector2i(xValue, yValue));
                            }
                            else
                            {
                                chunkInfo.chunks.Add(new Vector2i(xValue, yValue));
                            }
                        }
                    }
                    else
                    {
                        Debug.Log(entry.Key);
                    }
                    using (var fs = new System.IO.FileStream(realFilePath, FileMode.Create, System.IO.FileAccess.Write))
                    using (var ms = gameData.GetFileData(entry.Value))
                    {
                        ms?.WriteTo(fs);
                    }
                }

                var options = new JsonSerializerOptions { WriteIndented = true };
                string data = JsonSerializer.Serialize(chunkInfo, options);
                File.WriteAllText(newMap.mapChunkInfoPath, data);
            }

            ProjectManager.project?.Maps?.Add(newMap);
            ProjectManager.SaveProject();

            // Select last index
            this.worldManagerPane.mapComboBox.SelectedIndex = this.worldManagerPane.mapComboBox.Items.Count - 1;
        }

        internal void ImportLocalMap(string text)
        {
            
        }

        internal void LoadWorld(Engine.Project.Project.Map? map, MapChunkInfo? chunkInfo, Vector3 position)
        {
            if (map == null) return;
            if (chunkInfo == null) return;

            string projectFolder = $"{Path.GetDirectoryName(ProjectManager.projectFile)}\\{Path.GetFileNameWithoutExtension(ProjectManager.projectFile)}";
            this.engine.LoadWorld(map.worldRecord.ID, projectFolder, map.worldRecord.assetPath, map.Name, chunkInfo.chunks, position);
        }

        public void OnExit()
        {
            SaveMapPosition();
        }

        internal void SaveMapPosition()
        {
            if (this.worldManagerPane != null)
            {
                uint selectedMapID = this.worldManagerPane.selectedMapID;
                if (this.engine?.world?.renderer?.viewports?[0]?.mainCamera != null)
                {
                    for (int i = 0; i < ProjectManager.project?.Maps?.Count; i++)
                    {
                        if (ProjectManager.project.Maps[i].worldRecord.ID == selectedMapID)
                        {
                            ProjectManager.project.Maps[i].lastPosition = this.engine.world.renderer.viewports[0].mainCamera.transform.GetPosition();
                            ProjectManager.project.Maps[i].lastOrientation = this.engine.world.renderer.viewports[0].mainCamera.transform.GetRotation();
                            ProjectManager.SaveProject();
                            break;
                        }
                    }
                }
            }
        }

        public void CopyChunk(Vector2i fromLocation)
        {
            //throw new NotImplementedException();
        }

        internal void PasteChunk(Vector2i toLocation)
        {
            //throw new NotImplementedException();
        }

        internal void DeleteChunk(Vector2i atLocation)
        {
            //throw new NotImplementedException();
        }

        public float GetRoughHeightAtLocation(Vector3 worldCoord)
        {
            var chunkCoord = Utilities.WorldToChunkCoords(worldCoord);
            string xHex = chunkCoord.X.ToString("X2").ToLower();
            string yHex = chunkCoord.Y.ToString("X2").ToLower();
            string projectFolder = $"{Path.GetDirectoryName(ProjectManager.projectFile)}\\{Path.GetFileNameWithoutExtension(ProjectManager.projectFile)}";
            int mapIndexInProject = -1;
            for (int i = 0; i < ProjectManager.project?.Maps?.Count; i++)
            {
                if (ProjectManager.project.Maps[i].worldRecord.ID == this.worldManagerPane.selectedMapID)
                {
                    mapIndexInProject = i;
                    break;
                }
            }

            if (mapIndexInProject != -1)
            {
                Engine.Project.Project.Map map = ProjectManager.project.Maps[mapIndexInProject];
                string path = $"{projectFolder}\\{map.worldRecord.assetPath}\\{map.Name}.{yHex}{xHex}.area";
                FileFormats.Area.File aFile = new FileFormats.Area.File(path);

                using(var fs = File.OpenRead(path))
                {
                    aFile.Read(fs);
                }

                float scDist = float.MaxValue;
                int scIdx = 0;
                ushort maxH = ushort.MinValue;
                for (int s = 0; s < aFile.subAreas?.Count; s++)
                {
                    for (int i = 0; i < aFile.subAreas[s]?.heightMap?.Length; i++)
                    {
                        if (aFile.subAreas[s].heightMap[i] > maxH)
                            maxH = aFile.subAreas[s].heightMap[i];
                    }
                }

                // In the situation where terrain was never painted in the area, set height to 0
                if (maxH == ushort.MinValue)
                    maxH = 0;

                float h = ((maxH & 0x7FFF) * 0.12500381f) - 2048.0f;
                return h;
            }

            return 500;
            /*
            if (this.engine?.world?.chunks != null)
            {
                var chunkCoord = Utilities.WorldToChunkCoords(worldCoord);
                if (this.engine.world.chunks.TryGetValue(chunkCoord, out Chunk? chunk))
                {
                    if (chunk.lod0Available)
                    {
                        float scDist = float.MaxValue;
                        int scIdx = 0;
                        for (int s = 0; s < chunk.subChunks?.Count; s++)
                        {
                            var dist = Vector2.Distance(chunk.subChunks[s].centerPosition.Xz, worldCoord.Xz);
                            if (dist < scDist)
                            {
                                scDist = dist;
                                scIdx = s;
                            }
                        }

                        return chunk.subChunks[scIdx].centerPosition.Y;
                    }
                }
            }
            // Chunk isn't loaded so load area
            return 0;
            */
        }
    }
}