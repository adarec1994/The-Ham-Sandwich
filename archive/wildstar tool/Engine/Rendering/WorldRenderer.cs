using OpenTK.Graphics.OpenGL4;
using MathUtils;
using ProjectWS.Engine.Objects.Gizmos;

namespace ProjectWS.Engine.Rendering
{
    public class WorldRenderer : Renderer
    {
        public MousePick? mousePick;
        public World.World? world;
        private Engine engine;
        public ShaderParams.FogParameters? fogParameters;
        public ShaderParams.TerrainEditorParameters? tEditorParameters;
        public ShaderParams.SunParameters? sunParameters;
        public ShaderParams.EnvironmentParameters? envParameters;
        public ShaderParams.BrushParameters? brushParameters;
        public TextRenderer? textRenderer;
        private IconRenderer iconRenderer;
        public ImmediateRenderer? immRenderer;

        public static int drawCalls;
        public static int propDrawCalls;

        Color envColor = new Color(0.1f, 0.1f, 0.1f, 1.0f);
        public static Matrix4 decompressMat = Matrix4.CreateScale(1.0f / 1024.0f);

        public WorldRenderer(Engine engine, int ID, Input.Input input) : base(engine)
        {
            this.engine = engine;
            Debug.Log("Create World Renderer " + ID);
            this.ID = ID;
            this.input = input;
            //this.objects = new List<Objects.GameObject>();
            //this.lights = new List<Lighting.Light>();
            SetViewportMode(ViewMode.Default);
            //AddDefaultLight();
            //this.fogParameters = new FogParameters(this.envColor, 0, 1000, 0.1f, 0);  // Linear
            this.fogParameters = new ShaderParams.FogParameters(this.envColor, 0, 0, 0.0025f, 2);    // Exponential
            this.tEditorParameters = new ShaderParams.TerrainEditorParameters(false, false);
            var sunVector = new Vector3(1.0f, 1.0f, 1.0f);
            sunVector.Normalize();
            this.sunParameters = new ShaderParams.SunParameters(new Vector3(1.0f, 1.0f, 0.9f), sunVector, 1.0f);
            this.envParameters = new ShaderParams.EnvironmentParameters(new Vector3(0.4f, 0.4f, 0.6f));
            this.brushParameters = new ShaderParams.BrushParameters(ShaderParams.BrushParameters.BrushMode.Gradient, Vector3.Zero, 64.0f);

            // Initialize immediate mode debug renderers
            this.textRenderer = new TextRenderer();
            this.iconRenderer = new IconRenderer();
            this.immRenderer = new ImmediateRenderer();
            Debug.textRenderer = this.textRenderer;
            Debug.iconRenderer = this.iconRenderer;
            Debug.immRenderer = this.immRenderer;
        }

        public void SetWorld(World.World world) => this.world = world;

        public override void Load()
        {
            this.modelShader = new Shader("shaders/shader_vert.glsl", "shaders/shader_frag.glsl");
            this.shader = this.modelShader;
            this.wireframeShader = new Shader("shaders/wireframe_vert.glsl", "shaders/wireframe_frag.glsl");
            this.normalShader = new Shader("shaders/normal_vert.glsl", "shaders/normal_frag.glsl");
            this.terrainShader = new Shader("shaders/terrain_deferred_vert.glsl", "shaders/terrain_deferred_frag.glsl");
            this.waterShader = new Shader("shaders/water_vert.glsl", "shaders/water_frag.glsl");
            this.lineShader = new Shader("shaders/line_vert.glsl", "shaders/line_frag.glsl");
            this.fontShader = new Shader("shaders/font_vert.glsl", "shaders/font_frag.glsl");
            this.iconShader = new Shader("shaders/icon_vert.glsl", "shaders/icon_frag.glsl");
            this.lightPassShader = new Shader("shaders/light_pass_vert.glsl", "shaders/light_pass_frag.glsl");

            this.mousePick = new MousePick(this, this.engine);

            this.textRenderer?.Initialize();
            this.iconRenderer?.Initialize();
            this.immRenderer?.Initialize(this.lineShader);
            BuildGBufferQuad();
        }

        public override void Render(int frameBuffer)
        {
            if (this.viewports == null) return;

            drawCalls = 0;
            propDrawCalls = 0;

            GL.ClearColor(this.envColor.R, this.envColor.G, this.envColor.B, this.envColor.A);
            GL.Clear(ClearBufferMask.ColorBufferBit | ClearBufferMask.DepthBufferBit);
            GL.PolygonMode(MaterialFace.FrontAndBack, PolygonMode.Fill);

            // 1. geometry pass: render scene's geometry/color data into gbuffer
            // -----------------------------------------------------------------
            GL.BindFramebuffer(FramebufferTarget.Framebuffer, this.gBuffer);
            GL.Clear(ClearBufferMask.ColorBufferBit | ClearBufferMask.DepthBufferBit);

            for (int v = 0; v < this.viewports.Count; v++)
            {
                this.viewports[v].Use();

                // Set global shader parameters
                if (Engine.settings != null && Engine.settings.wRenderer != null && Engine.settings.wRenderer.toggles != null)
                {
                    this.fogParameters?.Toggle(Engine.settings.wRenderer.toggles.fog);
                    if (this.tEditorParameters != null)
                    {
                        this.tEditorParameters.enableAreaGrid = Engine.settings.wRenderer.toggles.displayAreaGrid;
                        this.tEditorParameters.enableChunkGrid = Engine.settings.wRenderer.toggles.displayChunkGrid;
                    }
                }

                // Render World
                if (this.world != null)
                {
                    // Terrain
                    this.terrainShader.Use();

                    this.tEditorParameters?.SetToShader(this.terrainShader);
                    //this.fogParameters.SetToShader(this.terrainShader);
                    //this.sunParameters.SetToShader(this.terrainShader);
                    //this.envParameters.SetToShader(this.terrainShader);
                    this.brushParameters?.SetToShader(this.terrainShader);
                    this.viewports[v].mainCamera.SetToShader(this.terrainShader);

                    this.world.RenderTerrain(this.terrainShader);

                    // Water
                    this.waterShader.Use();
                    this.viewports[v].mainCamera.SetToShader(this.waterShader);
                    var mat = Matrix4.Identity;
                    this.waterShader.SetMat4("model", ref mat);    // Water vertices are in world space
                    this.world.RenderWater(this.waterShader);

                    // Props
                    this.modelShader.Use();
                    this.viewports[v].mainCamera.SetToShader(this.modelShader);
                    this.envParameters?.SetToShader(this.modelShader);
                    this.world.RenderProps(this.modelShader);

                    GL.PolygonMode(MaterialFace.FrontAndBack, PolygonMode.Fill);
                    GL.Disable(EnableCap.Blend);
                }

                // Render Text
                this.textRenderer?.Render(this, this.viewports[v]);
                // Render Icons
                this.iconRenderer?.Render(this, this.viewports[v]);

                GL.Enable(EnableCap.DepthTest);

                // Render immediate mode
                this.immRenderer?.Render(this, this.viewports[v]);

                // Render Gizmos
                for (int i = 0; i < this.gizmos?.Count; i++)
                {
                    if (this.gizmos[i] == null) continue;

                    if (this.gizmos[i].visible == false) continue;

                    if (this.gizmos[i] is Objects.Gizmos.CameraGizmo)
                    {
                        var camGizmo = this.gizmos[i] as Objects.Gizmos.CameraGizmo;
                        if (camGizmo != null)
                            if (camGizmo.camera == this.viewports[v].mainCamera)
                                continue;
                    }

                    this.lineShader.Use();
                    this.viewports[v].mainCamera.SetToShader(this.lineShader);
                    this.gizmos[i].Render(Matrix4.Identity, this.lineShader);
                }
            }

            GL.BindFramebuffer(FramebufferTarget.Framebuffer, frameBuffer);

            GL.Viewport(this.x, this.y, this.width, this.height);

            // 2. lighting pass: calculate lighting by iterating over a screen filled quad pixel-by-pixel using the gbuffer's content.
            // -----------------------------------------------------------------------------------------------------------------------
            GL.Clear(ClearBufferMask.ColorBufferBit | ClearBufferMask.DepthBufferBit);
            this.lightPassShader.Use();

            GL.ActiveTexture(TextureUnit.Texture0);
            GL.BindTexture(TextureTarget.Texture2D, this.gDiffuse);
            GL.ActiveTexture(TextureUnit.Texture1);
            GL.BindTexture(TextureTarget.Texture2D, this.gSpecular);
            GL.ActiveTexture(TextureUnit.Texture2);
            GL.BindTexture(TextureTarget.Texture2D, this.gNormal);
            GL.ActiveTexture(TextureUnit.Texture3);
            GL.BindTexture(TextureTarget.Texture2D, this.gMisc);

            // send light relevant uniforms
            this.viewports[0].mainCamera.SetToShader(this.lightPassShader);
            this.fogParameters?.SetToShader(this.lightPassShader);
            this.sunParameters?.SetToShader(this.lightPassShader);
            this.envParameters?.SetToShader(this.lightPassShader);

            //shaderLightingPass.setVec3("viewPos", camera.Position);
            // finally render quad
            RenderQuad();

            // 2.5. copy content of geometry's depth buffer to default framebuffer's depth buffer
            // ----------------------------------------------------------------------------------
            GL.BindFramebuffer(FramebufferTarget.ReadFramebuffer, this.gBuffer);
            GL.BindFramebuffer(FramebufferTarget.DrawFramebuffer, 0);   // write to default framebuffer
                                                                        // blit to default framebuffer. Note that this may or may not work as the internal formats of both the FBO and default framebuffer have to match.
                                                                        // the internal formats are implementation defined. This works on all of my systems, but if it doesn't on yours you'll likely have to write to the 		
                                                                        // depth buffer in another shader stage (or somehow see to match the default framebuffer's internal format with the FBO's internal format).
            GL.BlitFramebuffer(0, 0, this.width, this.height, 0, 0, this.width, this.height, ClearBufferMask.DepthBufferBit, BlitFramebufferFilter.Nearest);
            //GL.BindFramebuffer(FramebufferTarget.Framebuffer, 0);
        }

        public override void Update(float deltaTime)
        {
            if (this.viewports == null) return;

            for (int v = 0; v < this.viewports.Count; v++)
            {
                this.viewports[v].mainCamera.Update(deltaTime);
                for (int c = 0; c < this.viewports[v].mainCamera.components.Count; c++)
                {
                    if (this.viewports[v].interactive)
                        this.viewports[v].mainCamera.components[c].Update(deltaTime);
                }
            }

            // Temp : Updating topdown camera manually
            if (this.viewports.Count == 2)
            {
                var p = this.viewports[0].mainCamera.transform.GetPosition() + new Vector3(0, 200, 0);
                this.viewports[1].mainCamera.view = Matrix4.LookAt(p, p - Vector3.UnitY, Vector3.UnitZ);
                this.viewports[1].mainCamera.transform.SetPosition(p);
            }

            this.world?.Update(deltaTime);

            if (this.mousePick != null && this.mousePick.mode != MousePick.Mode.Disabled)
                this.mousePick.Update();

            if (this.brushParameters != null && this.mousePick != null)
                this.brushParameters.position = this.mousePick.terrainHitPoint;
        }
    }
}
