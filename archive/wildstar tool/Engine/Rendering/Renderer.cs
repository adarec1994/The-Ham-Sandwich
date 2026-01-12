using ProjectWS.Engine.Input;
using MathUtils;
using ProjectWS.Engine.Components;
using OpenTK.Graphics.OpenGL4;

namespace ProjectWS.Engine.Rendering
{
    public abstract class Renderer
    {
        public int ID;
        public Engine engine;
        public Input.Input input;
        public bool rendering = false;
        public int x;
        public int y;
        public int width;
        public int height;

        public Shader shader;
        public Shader modelShader;
        public Shader wireframeShader;
        public Shader normalShader;
        public Shader terrainShader;
        public Shader waterShader;
        public Shader lineShader;
        public Shader infiniteGridShader;
        public Shader fontShader;
        public Shader iconShader;
        public Shader lightPassShader;
        public Shader mapTileShader;
        public Shader marqueeShader;

        public List<Viewport>? viewports;
        public List<Objects.GameObject>? gizmos;
        public ShadingOverride shadingOverride;
        public ViewMode viewportMode;

        public int gbufferQuadVAO = 0;
        public int gbufferQuadVBO;

        // Frame buffers
        public int gBuffer;
        public int gDiffuse, gSpecular, gNormal, gMisc;
        public int rboDepth;

        public Renderer(Engine engine)
        {
            this.engine = engine;
            this.gizmos = new List<Objects.GameObject>();
            this.viewports = new List<Viewport>();
        }

        public abstract void Load();
        public abstract void Update(float deltaTime);
        public abstract void Render(int frameBuffer);

        public void SetDimensions(int x, int y, int width, int height)
        {
            this.rendering = true;
            this.x = x;
            this.y = y;
            this.width = width;
            this.height = height;
            SetViewportMode(this.viewportMode);
        }

        public void Resize(int width, int height)
        {
            if (width == 0 || height == 0)
                return;

            Debug.Log("Resize " + width + " " + height);

            this.width = width;
            this.height = height;

            RecalculateViewports();
            ConfigureGBuffer(width, height);
        }

        void ConfigureGBuffer(int width, int height)
        {
            // Configure G-buffer
            if (this.gBuffer == 0)
                this.gBuffer = GL.GenFramebuffer();

            GL.BindFramebuffer(FramebufferTarget.Framebuffer, this.gBuffer);

            // Diffuse //
            if (this.gDiffuse == 0)
                this.gDiffuse = GL.GenTexture();
            GL.BindTexture(TextureTarget.Texture2D, gDiffuse);
            GL.TexImage2D(TextureTarget.Texture2D, 0, PixelInternalFormat.Rgba, width, height, 0, PixelFormat.Rgba, PixelType.UnsignedByte, IntPtr.Zero);
            GL.TexParameter(TextureTarget.Texture2D, TextureParameterName.TextureMinFilter, (int)TextureMinFilter.Nearest);
            GL.TexParameter(TextureTarget.Texture2D, TextureParameterName.TextureMagFilter, (int)TextureMagFilter.Nearest);
            GL.FramebufferTexture2D(FramebufferTarget.Framebuffer, FramebufferAttachment.ColorAttachment0, TextureTarget.Texture2D, gDiffuse, 0);

            // Specular //
            if (this.gSpecular == 0)
                this.gSpecular = GL.GenTexture();
            GL.BindTexture(TextureTarget.Texture2D, gSpecular);
            GL.TexImage2D(TextureTarget.Texture2D, 0, PixelInternalFormat.Rgba, width, height, 0, PixelFormat.Rgba, PixelType.UnsignedByte, IntPtr.Zero);
            GL.TexParameter(TextureTarget.Texture2D, TextureParameterName.TextureMinFilter, (int)TextureMinFilter.Nearest);
            GL.TexParameter(TextureTarget.Texture2D, TextureParameterName.TextureMagFilter, (int)TextureMagFilter.Nearest);
            GL.FramebufferTexture2D(FramebufferTarget.Framebuffer, FramebufferAttachment.ColorAttachment1, TextureTarget.Texture2D, gSpecular, 0);

            // Normal //
            if (this.gNormal == 0)
                this.gNormal = GL.GenTexture();
            GL.BindTexture(TextureTarget.Texture2D, this.gNormal);
            GL.TexImage2D(TextureTarget.Texture2D, 0, PixelInternalFormat.Rgba, width, height, 0, PixelFormat.Rgba, PixelType.UnsignedByte, IntPtr.Zero);
            GL.TexParameter(TextureTarget.Texture2D, TextureParameterName.TextureMinFilter, (int)TextureMinFilter.Nearest);
            GL.TexParameter(TextureTarget.Texture2D, TextureParameterName.TextureMagFilter, (int)TextureMagFilter.Nearest);
            GL.FramebufferTexture2D(FramebufferTarget.Framebuffer, FramebufferAttachment.ColorAttachment2, TextureTarget.Texture2D, this.gNormal, 0);

            // Unknown //
            if (this.gMisc == 0)
                this.gMisc = GL.GenTexture();
            GL.BindTexture(TextureTarget.Texture2D, gMisc);
            GL.TexImage2D(TextureTarget.Texture2D, 0, PixelInternalFormat.Rgba, width, height, 0, PixelFormat.Rgba, PixelType.UnsignedByte, IntPtr.Zero);
            GL.TexParameter(TextureTarget.Texture2D, TextureParameterName.TextureMinFilter, (int)TextureMinFilter.Nearest);
            GL.TexParameter(TextureTarget.Texture2D, TextureParameterName.TextureMagFilter, (int)TextureMagFilter.Nearest);
            GL.FramebufferTexture2D(FramebufferTarget.Framebuffer, FramebufferAttachment.ColorAttachment3, TextureTarget.Texture2D, gMisc, 0);

            // tell OpenGL which color attachments we'll use (of this framebuffer) for rendering 
            DrawBuffersEnum[] attachments = new DrawBuffersEnum[] { DrawBuffersEnum.ColorAttachment0, DrawBuffersEnum.ColorAttachment1, DrawBuffersEnum.ColorAttachment2, DrawBuffersEnum.ColorAttachment3 };
            GL.DrawBuffers(4, attachments);

            // create and attach depth buffer (renderbuffer)
            if (this.rboDepth == 0)
                this.rboDepth = GL.GenRenderbuffer();
            GL.BindRenderbuffer(RenderbufferTarget.Renderbuffer, rboDepth);
            GL.RenderbufferStorage(RenderbufferTarget.Renderbuffer, RenderbufferStorage.DepthComponent, width, height);
            GL.FramebufferRenderbuffer(FramebufferTarget.Framebuffer, FramebufferAttachment.DepthAttachment, RenderbufferTarget.Renderbuffer, rboDepth);

            // finally check if framebuffer is complete
            if (GL.CheckFramebufferStatus(FramebufferTarget.Framebuffer) != FramebufferErrorCode.FramebufferComplete)
                Debug.LogWarning("Framebuffer not complete!");

            GL.BindFramebuffer(FramebufferTarget.Framebuffer, 0);
        }

        public void BuildGBufferQuad()
        {
            if (this.gbufferQuadVAO == 0)
            {
                float[] quadVertices = new float[]{
                    // positions        // texture Coords
                    -1.0f,  1.0f, 0.0f, 0.0f, 1.0f,
                    -1.0f, -1.0f, 0.0f, 0.0f, 0.0f,
                     1.0f,  1.0f, 0.0f, 1.0f, 1.0f,
                     1.0f, -1.0f, 0.0f, 1.0f, 0.0f,
                };
                // setup plane VAO
                this.gbufferQuadVAO = GL.GenVertexArray();
                this.gbufferQuadVBO = GL.GenBuffer();
                GL.BindVertexArray(this.gbufferQuadVAO);
                GL.BindBuffer(BufferTarget.ArrayBuffer, this.gbufferQuadVBO);
                GL.BufferData(BufferTarget.ArrayBuffer, 4 * quadVertices.Length, quadVertices, BufferUsageHint.StaticDraw);
                GL.EnableVertexAttribArray(0);
                GL.VertexAttribPointer(0, 3, VertexAttribPointerType.Float, false, 5 * sizeof(float), 0);
                GL.EnableVertexAttribArray(1);
                GL.VertexAttribPointer(1, 2, VertexAttribPointerType.Float, false, 5 * sizeof(float), 3 * 4);
            }
        }

        public void SetShadingOverride(int type) => SetShadingOverride((ShadingOverride)type);

        public void SetShadingOverride(ShadingOverride type)
        {
            this.shadingOverride = type;
        }

        public void SetViewportMode(int mode) => SetViewportMode((ViewMode)mode);

        public void SetViewportMode(ViewMode mode)
        {
            if (this.viewports == null)
                this.viewports = new List<Viewport>();

            this.viewportMode = mode;

            if (mode == ViewMode.Default)
            {
                // Full view

                // Save camera controller
                Vector3 camPos = SaveCameraController();

                if (this.viewports.Count > 0)
                {
                    camPos = this.viewports[0].mainCamera.transform.GetPosition();
                    Debug.Log("Set Viewport Mode " + mode + " | Saved Pos " + camPos.ToString());
                }

                ClearViewports();

                if (this is ModelRenderer)
                {
                    this.viewports.Add(new Viewport(this, this.input, this.x, this.y, this.width, this.height, true, CameraController.Mode.Orbit));
                }
                else if (this is WorldRenderer)
                {
                    this.viewports.Add(new Viewport(this, this.input, this.x, this.y, this.width, this.height, true, CameraController.Mode.Fly));
                }
                else if (this is MapRenderer)
                {
                    this.viewports.Add(new Viewport(this, this.input, this.x, this.y, this.width, this.height, true, CameraController.Mode.OrthoTop));
                }

                // Restore camera controller
                RestoreCameraController(camPos);
            }
            else if (mode == ViewMode.SideBySide)
            {
                // Save camera controller
                Vector3 camPos = SaveCameraController();

                ClearViewports();

                // Side by side
                this.viewports.Add(new Viewport(this, this.input, this.x, this.y, this.width / 2, this.height, true, CameraController.Mode.Fly));
                this.viewports.Add(new Viewport(this, this.input, this.width / 2, this.y, this.width / 2, this.height, false, CameraController.Mode.OrthoTop));

                // Restore camera controller
                RestoreCameraController(camPos);

                // Temp : Hard coded map camera settings
                this.viewports[1].mainCamera.farDistance = 10000.0f;
                this.viewports[1].mainCamera.transform.SetPosition(camPos.X, 200, camPos.Z);
                this.viewports[1].mainCamera.transform.SetRotation(Quaternion.FromEulerAngles(MathHelper.DegreesToRadians(90), 0, 0));
            }
        }

        Vector3 SaveCameraController()
        {
            if (this.viewports != null)
            {
                if (this.viewports.Count > 0 && this.viewports[0].mainCamera != null && this.viewports[0].mainCamera.components != null)
                {
                    for (int i = 0; i < this.viewports[0].mainCamera.components.Count; i++)
                    {
                        if (this.viewports[0].mainCamera.components[i] is CameraController)
                        {
                            var camController = this.viewports[0].mainCamera.components[i] as CameraController;
                            if (camController != null)
                                return camController.Pos;
                        }
                    }
                }
            }

            return Vector3.Zero;
        }

        void RestoreCameraController(Vector3 camPos)
        {
            if (this.viewports == null) return;

            if (this.viewports.Count > 0 && this.viewports[0].mainCamera != null && this.viewports[0].mainCamera.components != null)
            {
                for (int i = 0; i < this.viewports[0].mainCamera.components.Count; i++)
                {
                    if (this.viewports[0].mainCamera.components[i] is CameraController)
                    {
                        var camController = this.viewports[0].mainCamera.components[i] as CameraController;
                        if (camController != null)
                            camController.Pos = camPos;
                    }
                }
            }
        }

        void ClearViewports()
        {
            if (this.viewports == null)
                this.viewports = new List<Viewport>();

            for (int v = 0; v < this.viewports.Count; v++)
            {
                var vp = this.viewports[v];

                if (vp.mainCamera != null)
                {
                    if (vp.mainCamera.gizmo != null && this.gizmos != null)
                    {
                        this.gizmos.Remove(vp.mainCamera.gizmo);
                    }
                }

            }

            this.viewports.Clear();
        }

        // renderQuad() renders a 1x1 XY quad in NDC
        // -----------------------------------------
        public void RenderQuad()
        {
            GL.BindVertexArray(this.gbufferQuadVAO);
            GL.DrawArrays(PrimitiveType.TriangleStrip, 0, 4);
            GL.BindVertexArray(0);
        }

        public void RecalculateViewports()
        {
            if (this.viewports == null) return;

            switch(this.viewportMode)
            {
                case ViewMode.Default:
                    this.viewports[0].Recalculate(this.x, this.y, this.width, this.height);
                    break;
                case ViewMode.SideBySide:
                    this.viewports[0].Recalculate(this.x, this.y, this.width / 2, this.height);
                    this.viewports[1].Recalculate(this.width / 2, this.y, this.width / 2, this.height);
                    break;
            }
        }

        public void ToggleFog(bool on)
        {
            if (Engine.settings != null && Engine.settings.wRenderer != null && Engine.settings.wRenderer.toggles != null)
            {
                Debug.Log("FOG " + on);
                Engine.settings.wRenderer.toggles.fog = on;
                SettingsSerializer.Save();
            }
        }

        public void ToggleAreaGrid(bool on)
        {
            if (Engine.settings != null && Engine.settings.wRenderer != null && Engine.settings.wRenderer.toggles != null)
            {
                Debug.Log("AREA GRID " + on);
                Engine.settings.wRenderer.toggles.displayAreaGrid = on;
                SettingsSerializer.Save();
            }
        }

        public void ToggleChunkGrid(bool on)
        {
            if (Engine.settings != null && Engine.settings.wRenderer != null && Engine.settings.wRenderer.toggles != null)
            {
                Debug.Log("CHUNK GRID " + on);
                Engine.settings.wRenderer.toggles.displayChunkGrid = on;
                SettingsSerializer.Save();
            }
        }

        public enum ShadingOverride
        {
            Shaded = 0,
            Unshaded = 1,
            Wireframe = 2,
            ShadedAndWireframe = 3,
            Normals = 4,
        }

        public enum ViewMode
        {
            Default = 0,
            SideBySide = 1,
        }
    }
}
