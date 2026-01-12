using MathUtils;
using OpenTK.Graphics.OpenGL4;
using ProjectWS.Engine.Components;
using ProjectWS.Engine.Database.Definitions;
using ProjectWS.Engine.TaskManager;
using ProjectWS.Engine.World;

namespace ProjectWS.Engine.Rendering
{
    public class MapRenderer : Renderer
    {
        public TextRenderer? textRenderer;
        public Vector2i highlight;
        public Vector2 mousePosMapSpace;
        public bool mouseOverGrid;
        public Vector2i selectedCell;
        byte[]? selectionBitmap;
        byte[]? availableBitmap;
        int bgVAO;
        int selectionLayerVAO;
        int abailableLayerVAO;
        int[]? gridIndices;
        int gridVAO;
        int minimapQuadVAO;
        private uint selectionBitmapPtr;
        private uint availableBitmapPtr;
        private bool marqueeVisible;
        public Vector2 mapLMBMin;
        public Vector2 mapLMBMax;
        public Vector2 mapLMBMaxPrevious;
        public Vector2 screenRMBMin;
        public Vector2 screenRMBMax;
        public Vector2 mapRMBMin;
        public Vector2 mapRMBMax;
        private int[]? lineSquareIndices;
        private int lineSquareVAO;
        //public bool deselectMode;
        public bool showGrid = true;
        public bool mouseDownInView = false;
        List<Vector2i>? availableChunks;
        List<Vector2i>? availableMinimaps;
        MinimapChunk[][] minimaps;
        float zoomLevel;
        public bool singleSelect = true;
        public bool marqueeSelect = false;
        int cellSizeOnScreen = 0;

        public TThread minimapThread;


        const int MAP_SIZE = 128;
        readonly Color32 envColor = new Color32(10, 10, 20, 255);
        readonly Color32 selectedCellColor = new Color32(255, 255, 0, 50);
        readonly Color32 deselectedCellColor = new Color32(0, 0, 0, 0);
        readonly Color32 backgroundCellColor = new Color32(50, 50, 50, 255);
        readonly Color32 hasAreaColor = new Color32(0, 0, 0, 0);
        readonly Color32 noAreaColor = new Color32(0, 0, 0, 255);
        readonly Color32 gridColor = new Color32(20, 20, 20, 255);
        const float halfQuad = 0.5f;

        readonly float[] quadVertices = new float[]
        {
            // positions                // texture Coords
            -halfQuad, 0.0f, halfQuad,  0.0f, 1.0f,
            -halfQuad, 0.0f,-halfQuad,  0.0f, 0.0f,
             halfQuad, 0.0f, halfQuad,  1.0f, 1.0f,
             halfQuad, 0.0f,-halfQuad,  1.0f, 0.0f,
        };

        public Action<Vector2i>? onCellHighlight;
        public Action<float>? onZoomLevelChanged;
        public Action onRightClick;
        private string? projectFile;
        private string? assetPath;
        private string? mapName;

        public MapRenderer(Engine engine, int ID, Input.Input input) : base(engine)
        {
            Debug.Log("Create Map Renderer " + ID);
            this.ID = ID;
            this.input = input;

            SetViewportMode(0);

            this.minimapThread = new TThread("Minimap");

            this.textRenderer = new TextRenderer();
        }

        public override void Load()
        {
            this.mapTileShader = new Shader("shaders/maptile_vert.glsl", "shaders/maptile_frag.glsl");
            this.fontShader = new Shader("shaders/font_vert.glsl", "shaders/font_frag.glsl");
            this.lineShader = new Shader("shaders/line_vert.glsl", "shaders/line_frag.glsl");
            this.marqueeShader = new Shader("shaders/marquee_vert.glsl", "shaders/marquee_frag.glsl");

            // setup marqueue quad
            BuildLineSquare();

            // setup Background
            this.bgVAO = BuildQuad(quadVertices);

            // setup Selection Layer
            this.selectionLayerVAO = BuildQuad(quadVertices);

            this.abailableLayerVAO = BuildQuad(quadVertices);

            BuildGrid();

            this.minimapQuadVAO = BuildQuad(quadVertices);

            this.selectionBitmapPtr = BuildBitmap(this.deselectedCellColor, MAP_SIZE, MAP_SIZE, out this.selectionBitmap);

            this.availableBitmapPtr = BuildBitmap(this.noAreaColor, MAP_SIZE, MAP_SIZE, out this.availableBitmap);

            this.textRenderer?.Initialize();

            this.minimaps = new MinimapChunk[MAP_SIZE][];
            float minimapLayer = 6f / 1000f;
            for (int x = 0; x < MAP_SIZE; x++)
            {
                this.minimaps[x] = new MinimapChunk[MAP_SIZE];
                for (int y = 0; y < MAP_SIZE; y++)
                {
                    this.minimaps[x][y] = new MinimapChunk();
                    this.minimaps[x][y].matrix = Matrix4.CreateTranslation(x + 0.5f, minimapLayer, y + 0.5f);
                }
            }

            var vp = this.viewports![0];
            var oCamera = vp.mainCamera as OrthoCamera;
            (oCamera.components[0] as CameraController).Pos = new Vector3(64, 0, 64);

            RefreshMapView();
            RefreshMinimaps();
        }

        private int BuildQuad(float[] quadVertices)
        {
            int vao = GL.GenVertexArray();
            var vbo = GL.GenBuffer();
            GL.BindVertexArray(vao);
            GL.BindBuffer(BufferTarget.ArrayBuffer, vbo);
            GL.BufferData(BufferTarget.ArrayBuffer, 4 * quadVertices.Length, quadVertices, BufferUsageHint.StaticDraw);
            GL.EnableVertexAttribArray(0);
            GL.VertexAttribPointer(0, 3, VertexAttribPointerType.Float, false, 5 * sizeof(float), 0);
            GL.EnableVertexAttribArray(1);
            GL.VertexAttribPointer(1, 2, VertexAttribPointerType.Float, false, 5 * sizeof(float), 3 * 4);

            return vao;
        }

        public void BuildGrid()
        {
            var line = new Vector3[MAP_SIZE * 4];

            int idx = 0;
            for (int x = 0; x < MAP_SIZE; x++)
            {
                line[idx++] = new Vector3(x, 0, 0);
                line[idx++] = new Vector3(x, 0, MAP_SIZE);
                line[idx++] = new Vector3(0, 0, x);
                line[idx++] = new Vector3(MAP_SIZE, 0, x);
            }

            this.gridIndices = new int[MAP_SIZE * 4];
            for (int i = 0; i < MAP_SIZE * 4; i += 2)
            {
                this.gridIndices[i] = i;
                this.gridIndices[i + 1] = i + 1;
            }

            int _vertexBufferObject = GL.GenBuffer();
            GL.BindBuffer(BufferTarget.ArrayBuffer, _vertexBufferObject);
            GL.BufferData(BufferTarget.ArrayBuffer, line.Length * 3 * 4, line, BufferUsageHint.StaticDraw);

            this.gridVAO = GL.GenVertexArray();
            GL.BindVertexArray(this.gridVAO);

            GL.EnableVertexAttribArray(0);
            GL.VertexAttribPointer(0, 3, VertexAttribPointerType.Float, false, 0, 0);

            int _elementBufferObject = GL.GenBuffer();
            GL.BindBuffer(BufferTarget.ElementArrayBuffer, _elementBufferObject);
            GL.BufferData(BufferTarget.ElementArrayBuffer, this.gridIndices.Length * 4, this.gridIndices, BufferUsageHint.StaticDraw);

            GL.BindVertexArray(0);
        }

        public void BuildLineSquare()
        {
            var line = new Vector3[]
            {
                new Vector3(-0.5f, 0, -0.5f),
                new Vector3(-0.5f, 0, 0.5f),
                new Vector3(0.5f, 0, 0.5f),
                new Vector3(0.5f, 0, -0.5f),
            };

            this.lineSquareIndices = new int[]
            {
                0, 1, 1, 2, 2, 3, 3, 0
            };

            int _vertexBufferObject = GL.GenBuffer();
            GL.BindBuffer(BufferTarget.ArrayBuffer, _vertexBufferObject);
            GL.BufferData(BufferTarget.ArrayBuffer, line.Length * 3 * 4, line, BufferUsageHint.StaticDraw);

            this.lineSquareVAO = GL.GenVertexArray();
            GL.BindVertexArray(this.lineSquareVAO);

            GL.EnableVertexAttribArray(0);
            GL.VertexAttribPointer(0, 3, VertexAttribPointerType.Float, false, 0, 0);

            int _elementBufferObject = GL.GenBuffer();
            GL.BindBuffer(BufferTarget.ElementArrayBuffer, _elementBufferObject);
            GL.BufferData(BufferTarget.ElementArrayBuffer, this.lineSquareIndices.Length * 4, this.lineSquareIndices, BufferUsageHint.StaticDraw);

            GL.BindVertexArray(0);
        }

        uint BuildBitmap(Color32 baseColor, int w, int h, out byte[] data)
        {
            uint ptr;
            data = new byte[w * h * 4];
            for (int i = 0; i < w * h * 4; i += 4)
            {
                data[i] = baseColor.R;
                data[i + 1] = baseColor.G;
                data[i + 2] = baseColor.B;
                data[i + 3] = baseColor.A;
            }

            GL.GenTextures(1, out ptr);
            GL.BindTexture(TextureTarget.Texture2D, ptr);
            GL.TexParameter(TextureTarget.Texture2D, TextureParameterName.TextureWrapS, (int)TextureWrapMode.ClampToEdge);
            GL.TexParameter(TextureTarget.Texture2D, TextureParameterName.TextureWrapT, (int)TextureWrapMode.ClampToEdge);
            GL.TexParameter(TextureTarget.Texture2D, TextureParameterName.TextureMinFilter, (int)TextureMinFilter.Nearest);
            GL.TexParameter(TextureTarget.Texture2D, TextureParameterName.TextureMagFilter, (int)TextureMagFilter.Nearest);
            GL.TexParameter(TextureTarget.Texture2D, TextureParameterName.GenerateMipmap, 0);
            GL.TexParameter(TextureTarget.Texture2D, TextureParameterName.TextureBaseLevel, 0);
            GL.TexParameter(TextureTarget.Texture2D, TextureParameterName.TextureMaxLevel, 0);

            GL.TexImage2D(TextureTarget.Texture2D, 0, PixelInternalFormat.Rgba, w, h, 0, PixelFormat.Rgba, PixelType.UnsignedByte, data);

            return ptr;
        }

        public void UpdateBitmap(uint ptr, byte[]? data)
        {
            if (data == null) return;
            GL.BindTexture(TextureTarget.Texture2D, ptr);
            GL.TexImage2D(TextureTarget.Texture2D, 0, PixelInternalFormat.Rgba, MAP_SIZE, MAP_SIZE, 0, PixelFormat.Rgba, PixelType.UnsignedByte, data);
        }

        public override void Update(float deltaTime)
        {
            if (this.viewports == null) return;

            if (!this.minimapThread.isRunning)
            {
                if (this.minimapThread.tasks.Count > 0)
                {
                    this.minimapThread.Boot(0);
                }
            }

            for (int i = 0; i < this.viewports?.Count; i++)
            {
                this.viewports[i].mainCamera?.Update(deltaTime);
                for (int c = 0; c < this.viewports[i].mainCamera?.components.Count; c++)
                {
                    this.viewports[i].mainCamera?.components[c].Update(deltaTime);
                }
            }

            //textRenderer?.DrawLabel3D("TEST", new Vector3(0, 0.1f, 0), new Vector4(1, 1, 0, 1), true);

            // Determine which quad mouse is over
            var vp = this.viewports![0];
            var vpSize = new Vector2((float)vp.width, (float)vp.height);

            this.mouseOverGrid = false;

            if (this.ID == this.engine.focusedRendererID)
            {
                var oCamera = vp.mainCamera as OrthoCamera;

                // Mipmap cell culling
                MipmapCellCulling(oCamera, vpSize);

                // Calculate cell size on screen
                this.cellSizeOnScreen = (int)oCamera.zoom;

                // Map mouse interaction
                MapMousePosition(vp, oCamera, vpSize);
            }

            MapMouseInput(vpSize);
        }

        private void MapMouseInput(Vector2 vpSize)
        {
            if (this.engine.input.LMBClicked == Input.Input.ClickState.MouseButtonDown || this.engine.input.RMBClicked == Input.Input.ClickState.MouseButtonDown)
            {
                var mousePos = this.engine.input.GetMousePosition();

                if (mousePos.X >= 0 && mousePos.Y >= 0 && mousePos.X < vpSize.X && mousePos.Y < vpSize.Y)
                {
                    this.mouseDownInView = true;
                    this.input.mouseDownInView = true;

                    if (this.engine.input.LMBClicked == Input.Input.ClickState.MouseButtonDown)
                        this.mapLMBMin = mousePosMapSpace;
                }
                else
                {
                    this.mouseDownInView = false;
                    this.input.mouseDownInView = false;
                }

                if (this.engine.input.LMBClicked == Input.Input.ClickState.MouseButtonDown && this.marqueeSelect)
                {
                    this.marqueeVisible = true;
                }
            }

            if (this.engine.input.LMB)
            {
                this.mapLMBMax = this.mousePosMapSpace;

                if (this.mouseDownInView && this.marqueeSelect)
                {
                    if (this.mapLMBMax != this.mapLMBMaxPrevious)
                    {
                        this.mapLMBMaxPrevious = this.mapLMBMax;

                        var minX = MathF.Min(this.mapLMBMin.X, this.mapLMBMax.X);
                        var maxX = MathF.Max(this.mapLMBMin.X, this.mapLMBMax.X);
                        var minY = MathF.Min(this.mapLMBMin.Y, this.mapLMBMax.Y);
                        var maxY = MathF.Max(this.mapLMBMin.Y, this.mapLMBMax.Y);

                        DeselectAllCells();

                        for (int x = (int)minX; x <= (int)maxX; x++)
                        {
                            for (int y = (int)minY; y <= (int)maxY; y++)
                            {
                                SelectCell(x, y);
                            }
                        }

                        UpdateBitmap(this.selectionBitmapPtr, this.selectionBitmap);
                    }
                }
            }
            if (this.engine.input.LMBClicked == Input.Input.ClickState.MouseButtonUp)
            {
                this.marqueeVisible = false;

                if (this.mouseDownInView && this.singleSelect)
                {
                    DeselectAllCells();

                    //if (this.marqueeMin == this.marqueeMax)  // If you moved your mouse away from the first cell clicked then don't select it
                    {
                        SelectCell(this.highlight.X, this.highlight.Y);
                        selectedCell = this.highlight;
                    }

                    UpdateBitmap(this.selectionBitmapPtr, this.selectionBitmap);
                }
            }

            if (this.engine.input.RMBClicked == Input.Input.ClickState.MouseButtonDown)
            {
                if (this.mouseDownInView)
                {
                    this.screenRMBMin = this.engine.input.GetMousePosition().Xy;
                    this.mapRMBMin = this.mousePosMapSpace;
                }
            }

            if (this.engine.input.RMBClicked == Input.Input.ClickState.MouseButtonUp)
            {
                if (this.mouseDownInView)
                {
                    this.screenRMBMax = this.engine.input.GetMousePosition().Xy;
                    this.mapRMBMax = this.mousePosMapSpace;

                    var distance = Vector2.Distance(this.screenRMBMin, this.screenRMBMax);

                    if (distance < 2)  // Making sure mouse didn't move (much) while right click
                    {
                        this.onRightClick?.Invoke();
                    }
                }
            }
        }

        private void MapMousePosition(Viewport vp, OrthoCamera? oCamera, Vector2 vpSize)
        {
            if (vp.interactive)
            {
                var mousePos = this.engine.input.GetMousePosition();

                if (mousePos.X >= 0 && mousePos.Y >= 0 && mousePos.X < vpSize.X && mousePos.Y < vpSize.Y)
                {
                    this.mouseOverGrid = true;

                    if (oCamera != null)
                    {
                        if (this.zoomLevel != oCamera.zoom)
                        {
                            this.zoomLevel = oCamera.zoom;
                            this.onZoomLevelChanged?.Invoke(this.zoomLevel);
                        }

                        this.mousePosMapSpace = ((mousePos.Xy - (vpSize / 2)) / oCamera.zoom) + oCamera.transform.GetPosition().Xz;

                        Vector2i currentHighlight = new Vector2i((int)this.mousePosMapSpace.X, (int)this.mousePosMapSpace.Y);

                        //if (this.highlight != currentHighlight)
                        {
                            // Mouse over cell changed
                            this.onCellHighlight?.Invoke(this.highlight);
                        }

                        this.highlight = currentHighlight;

                        // Out of map bounds check
                        if (this.highlight.X < 0 || this.highlight.X >= MAP_SIZE || this.highlight.Y < 0 || this.highlight.Y >= MAP_SIZE)
                            this.mouseOverGrid = false;
                    }
                }
            }
        }

        private void MipmapCellCulling(OrthoCamera? oCamera, Vector2 vpSize)
        {
            var topLeft = ((Vector2.Zero - (vpSize / 2)) / oCamera.zoom) + oCamera.transform.GetPosition().Xz;
            var bottomRight = ((vpSize - (vpSize / 2)) / oCamera.zoom) + oCamera.transform.GetPosition().Xz;

            var topLeftI = new Vector2i((int)topLeft.X, (int)topLeft.Y);
            var bottomRightI = new Vector2i((int)bottomRight.X, (int)bottomRight.Y);

            for (int x = 0; x < MAP_SIZE; x++)
            {
                if (x >= topLeftI.X && x <= bottomRightI.X)
                {
                    for (int y = 0; y < MAP_SIZE; y++)
                    {
                        if (y >= topLeftI.Y && y <= bottomRightI.Y)
                        {
                            this.minimaps[x][y].isVisible = true;
                        }
                        else
                        {
                            this.minimaps[x][y].isVisible = false;
                        }
                    }
                }
                else
                {
                    for (int y = 0; y < MAP_SIZE; y++)
                    {
                        this.minimaps[x][y].isVisible = false;
                    }
                }
            }
        }

        bool AreVectorsDifferent(Vector2 a, Vector2 b)
        {
            if (a.X != b.X) return true;
            if (a.Y != b.Y) return true;
            return false;
        }

        public void SelectCell(int x, int y)
        {
            if (x < 0 || y < 0 || x >= MAP_SIZE || y >= MAP_SIZE)
                return;
            var linearCoord = (int)((y * 4 * MAP_SIZE) + (x * 4));
            if (this.selectionBitmap != null)
            {
                this.selectionBitmap[linearCoord] = selectedCellColor.R;
                this.selectionBitmap[linearCoord + 1] = selectedCellColor.G;
                this.selectionBitmap[linearCoord + 2] = selectedCellColor.B;
                this.selectionBitmap[linearCoord + 3] = selectedCellColor.A;
            }
        }

        public void DeselectCell(int x, int y)
        {
            if (x < 0 || y < 0 || x >= MAP_SIZE || y >= MAP_SIZE)
                return;
            var linearCoord = (int)((y * 4 * MAP_SIZE) + (x * 4));
            if (this.selectionBitmap != null)
            {
                this.selectionBitmap[linearCoord] = deselectedCellColor.R;
                this.selectionBitmap[linearCoord + 1] = deselectedCellColor.G;
                this.selectionBitmap[linearCoord + 2] = deselectedCellColor.B;
                this.selectionBitmap[linearCoord + 3] = deselectedCellColor.A;
            }
        }

        public void DeselectAllCells()
        {
            for (int x = 0; x < MAP_SIZE; x++)
            {
                for (int y = 0; y < MAP_SIZE; y++)
                {
                    DeselectCell(x, y);
                }
            }

            if (this.selectionBitmap != null)
                UpdateBitmap(this.selectionBitmapPtr, this.selectionBitmap);
        }

        public void RefreshMapView(List<Vector2i>? chunks = null)
        {
            if (chunks != null)
                this.availableChunks = chunks;
            else
                chunks = this.availableChunks;

            for (int x = 0; x < MAP_SIZE; x++)
            {
                for (int y = 0; y < MAP_SIZE; y++)
                {
                    MakeCellUnAvailable(x, y);
                }
            }

            if (chunks != null)
            {
                for (int i = 0; i < chunks.Count; i++)
                {
                    MakeCellAvailable(chunks[i].X, chunks[i].Y);
                }
            }

            UpdateBitmap(this.availableBitmapPtr, this.availableBitmap);
        }

        public void RefreshMinimaps(List<Vector2i>? availableMinimaps = null, string? projectFile = null, string? assetPath = null, string? mapName = null)
        {
            if (availableMinimaps != null)
                this.availableMinimaps = availableMinimaps;

            if (projectFile != null)
                this.projectFile = projectFile;

            if (assetPath != null)
                this.assetPath = assetPath;

            if (mapName != null)
                this.mapName = mapName;

            if (this.minimaps == null) return;
            if (this.availableMinimaps == null) return;

            HashSet<Vector2i> available = new HashSet<Vector2i>(this.availableMinimaps);

            string projectFolder = $"{Path.GetDirectoryName(this.projectFile)}\\{Path.GetFileNameWithoutExtension(this.projectFile)}";

            for (int x = 0; x < MAP_SIZE; x++)
            {
                for (int y = 0; y < MAP_SIZE; y++)
                {
                    if (available.Contains(new Vector2i(x, y)))
                    {
                        this.minimaps[x][y].exists = true;

                        string xHex = x.ToString("X2").ToLower();
                        string yHex = y.ToString("X2").ToLower();
                        
                        string path = $"{projectFolder}\\{this.assetPath}\\{this.mapName}.{yHex}{xHex}.tex";
                        this.minimaps[x][y].path = path;
                        FileFormats.Tex.File tex = new FileFormats.Tex.File(path);
                        this.minimaps[x][y].texFile = tex;
                    }
                    else
                    {
                        this.minimaps[x][y].exists = false;
                    }
                }
            }
        }

        public void MakeCellAvailable(int x, int y)
        {
            if (x < 0 || y < 0 || x >= MAP_SIZE || y >= MAP_SIZE)
                return;
            var linearCoord = (int)((y * 4 * MAP_SIZE) + (x * 4));

            if (this.availableBitmap != null)
            {
                this.availableBitmap[linearCoord] = hasAreaColor.R;
                this.availableBitmap[linearCoord + 1] = hasAreaColor.G;
                this.availableBitmap[linearCoord + 2] = hasAreaColor.B;
                this.availableBitmap[linearCoord + 3] = hasAreaColor.A;
            }
        }

        public void MakeCellUnAvailable(int x, int y)
        {
            if (x < 0 || y < 0 || x >= MAP_SIZE || y >= MAP_SIZE)
                return;
            var linearCoord = (int)((y * 4 * MAP_SIZE) + (x * 4));
            if (this.availableBitmap != null)
            {
                this.availableBitmap[linearCoord] = noAreaColor.R;
                this.availableBitmap[linearCoord + 1] = noAreaColor.G;
                this.availableBitmap[linearCoord + 2] = noAreaColor.B;
                this.availableBitmap[linearCoord + 3] = noAreaColor.A;
            }
        }

        public override void Render(int frameBuffer)
        {
            if (!this.rendering) return;

            //GL.Viewport(this.x, this.y, this.width, this.height);
            GL.ClearColor(envColor.R / 255f, envColor.G / 255f, envColor.B / 255f, envColor.A / 255f);
            GL.Clear(ClearBufferMask.ColorBufferBit | ClearBufferMask.DepthBufferBit);

            GL.PolygonMode(MaterialFace.FrontAndBack, PolygonMode.Fill);
            GL.Disable(EnableCap.Blend);
            GL.Enable(EnableCap.DepthTest);

            this.viewports?[0]?.Use();

            // Render BG
            RenderBackground(0);

            // Render available chunks layer
            RenderAvailableLayer(5);

            // render minimaps
            RenderMinimaps(6);

            // render selection
            RenderSelectionLayer(7);

            // Render grid
            if (showGrid)
                RenderGrid(19);

            // render marqueue
            RenderMarqueue(20);


            // Render Text
            //this.textRenderer?.Render(this, this.viewports![0]);
        }

        private void RenderMinimaps(int layer)
        {
            GL.Disable(EnableCap.Blend);

            this.mapTileShader.Use();
            this.viewports?[0]?.mainCamera?.SetToShader(this.mapTileShader);

            int mip;
            if (this.cellSizeOnScreen >= 512)
                mip = 0;
            else if (this.cellSizeOnScreen >= 256)
                mip = 1;
            else if (this.cellSizeOnScreen >= 128)
                mip = 2;
            else if (this.cellSizeOnScreen >= 64)
                mip = 3;
            else if (this.cellSizeOnScreen >= 32)
                mip = 4;
            else if (this.cellSizeOnScreen >= 16)
                mip = 5;
            else if (this.cellSizeOnScreen >= 8)
                mip = 6;
            else
                mip = 7;

            for (int x = 0; x < MAP_SIZE; x++)
            {
                for (int y = 0; y < MAP_SIZE; y++)
                {
                    var minimap = this.minimaps[x][y];
                    if (minimap.exists)
                    {
                        if (minimap.isVisible)
                        {
                            if (!minimap.isRead)
                            {
                                this.minimapThread.Enqueue(new TaskManager.MinimapTask(minimap));
                            }

                            if (minimap.isRead)
                            {
                                minimap.Render(this.mapTileShader, mip, this.minimapQuadVAO);
                            }
                        }
                    }
                }
            }

            GL.BindVertexArray(0);
        }

        private void RenderGrid(int layer)
        {
            GL.Disable(EnableCap.Blend);

            this.lineShader.Use();
            this.viewports?[0]?.mainCamera?.SetToShader(this.lineShader);
            var gridMat = Matrix4.CreateTranslation(0, layer/1000f, 0);
            this.lineShader.SetMat4("model", ref gridMat);
            this.lineShader.SetColor("lineColor", gridColor);
            GL.BindVertexArray(this.gridVAO);
            GL.DrawElements(BeginMode.Lines, this.gridIndices!.Length, DrawElementsType.UnsignedInt, 0);
        }

        private void RenderMarqueue(int layer)
        {
            if (this.marqueeVisible)
            {
                GL.Disable(EnableCap.Blend);

                this.marqueeShader.Use();
                this.viewports?[0]?.mainCamera?.SetToShader(this.marqueeShader);

                // Calculate the position and size of the quad
                Vector2 position = (this.mapLMBMin + this.mapLMBMax) / 2;
                Vector2 size = this.mapLMBMax - this.mapLMBMin;

                // Create the matrix that transforms the quad
                var quadMat = Matrix4.CreateScale(size.X, 1f, size.Y) * Matrix4.CreateTranslation(position.X, layer / 1000f, position.Y);

                this.marqueeShader.SetMat4("model", ref quadMat);
                this.marqueeShader.SetColor("lineColor", new Color(1f, 1f, 1f, 1.0f));
                this.marqueeShader.SetFloat("aspectRatio", this.viewports![0].aspect);

                GL.BindVertexArray(this.lineSquareVAO);
                GL.DrawElements(BeginMode.Lines, this.lineSquareIndices!.Length, DrawElementsType.UnsignedInt, 0);
            }
        }

        private void RenderBackground(int layer)
        {
            GL.Disable(EnableCap.Blend);

            this.lineShader.Use();
            this.viewports?[0]?.mainCamera?.SetToShader(this.lineShader);
            var bgMat = Matrix4.CreateScale(MAP_SIZE) * Matrix4.CreateTranslation(MAP_SIZE / 2, layer / 1000f, MAP_SIZE / 2);
            this.lineShader.SetMat4("model", ref bgMat);
            this.lineShader.SetColor("lineColor", backgroundCellColor);

            GL.BindVertexArray(this.bgVAO);
            GL.DrawArrays(PrimitiveType.TriangleStrip, 0, 4);
            GL.BindVertexArray(0);
        }

        private void RenderSelectionLayer(int layer)
        {
            GL.Enable(EnableCap.Blend);
            GL.BlendFunc(BlendingFactor.SrcAlpha, BlendingFactor.OneMinusSrcAlpha);

            this.mapTileShader.Use();
            this.viewports?[0]?.mainCamera?.SetToShader(this.mapTileShader);
            GL.ActiveTexture(TextureUnit.Texture0);
            GL.BindTexture(TextureTarget.Texture2D, this.selectionBitmapPtr);
            var bgMat = Matrix4.CreateScale(MAP_SIZE) * Matrix4.CreateTranslation(MAP_SIZE / 2, layer / 1000f, MAP_SIZE / 2);
            this.mapTileShader.SetMat4("model", ref bgMat);

            GL.BindVertexArray(this.selectionLayerVAO);
            GL.DrawArrays(PrimitiveType.TriangleStrip, 0, 4);
            GL.BindVertexArray(0);
        }

        private void RenderAvailableLayer(int layer)
        {
            GL.Enable(EnableCap.Blend);
            GL.BlendFunc(BlendingFactor.SrcAlpha, BlendingFactor.OneMinusSrcAlpha);

            this.mapTileShader.Use();
            this.viewports?[0]?.mainCamera?.SetToShader(this.mapTileShader);
            GL.ActiveTexture(TextureUnit.Texture0);
            GL.BindTexture(TextureTarget.Texture2D, this.availableBitmapPtr);
            var bgMat = Matrix4.CreateScale(MAP_SIZE) * Matrix4.CreateTranslation(MAP_SIZE / 2, layer / 1000f, MAP_SIZE / 2);
            this.mapTileShader.SetMat4("model", ref bgMat);

            GL.BindVertexArray(this.abailableLayerVAO);
            GL.DrawArrays(PrimitiveType.TriangleStrip, 0, 4);
            GL.BindVertexArray(0);
        }

        public void ClearMap()
        {
            if (minimapThread != null)
                minimapThread.Clear();

            this.availableChunks?.Clear();
            this.availableMinimaps?.Clear();


            for (int x = 0; x < MAP_SIZE; x++)
            {
                for (int y = 0; y < MAP_SIZE; y++)
                {
                    MakeCellUnAvailable(x, y);
                }
            }

            UpdateBitmap(this.availableBitmapPtr, this.availableBitmap);

            if (this.minimaps != null)
            {
                for (int x = 0; x < MAP_SIZE; x++)
                {
                    for (int y = 0; y < MAP_SIZE; y++)
                    {
                        this.minimaps[x][y].Clear();
                    }
                }
            }
        }
    }
}
