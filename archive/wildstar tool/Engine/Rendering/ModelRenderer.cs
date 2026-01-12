using MathUtils;
using OpenTK.Graphics.OpenGL4;

namespace ProjectWS.Engine.Rendering
{
    public class ModelRenderer : Renderer
    {
        public List<Objects.GameObject> objects;
        public List<Lighting.Light> lights;
        public Objects.Gizmos.AxisGizmo axisGizmo;

        public ModelRenderer(Engine engine, int ID, Input.Input input) : base(engine)
        {
            Debug.Log("Create Model Renderer " + ID);
            this.ID = ID;
            this.input = input;
            this.objects = new List<Objects.GameObject>();
            this.lights = new List<Lighting.Light>();

            SetViewportMode(0);
            AddDefaultLight();
            AddAxisGizmo();
        }

        public void AddModel(Objects.M3Model m3Model)
        {
            this.objects.Add(m3Model);
        }

        void AddAxisGizmo()
        {
            this.axisGizmo = new Objects.Gizmos.AxisGizmo();
            if (this.gizmos != null)
                this.gizmos.Add(this.axisGizmo);
            if (this.engine != null)
                this.engine.taskManager.buildTasks.Enqueue(new TaskManager.BuildObjectTask(this.axisGizmo));
        }

        void AddDefaultLight()
        {
            Vector3 lightPos = new Vector3(100f, 100f, 100f);
            Vector4 lightColor = new Vector4(1.0f, 1.0f, 1.0f, 1.0f);
            this.lights.Add(new Lighting.DirectLight(lightPos, lightColor));

            Vector4 ambientColor = new Vector4(0.2f, 0.2f, 0.2f, 1.0f);
            this.lights.Add(new Lighting.AmbientLight(ambientColor));
        }

        public override void Load()
        {
            this.modelShader = new Shader("shaders/shader_vert.glsl", "shaders/shader_frag.glsl");
            this.shader = this.modelShader;
            this.wireframeShader = new Shader("shaders/wireframe_vert.glsl", "shaders/wireframe_frag.glsl");
            this.normalShader = new Shader("shaders/normal_vert.glsl", "shaders/normal_frag.glsl");
            this.terrainShader = new Shader("shaders/terrain_vert.glsl", "shaders/terrain_frag.glsl");
            this.waterShader = new Shader("shaders/water_vert.glsl", "shaders/water_frag.glsl");
            this.lineShader = new Shader("shaders/line_vert.glsl", "shaders/line_frag.glsl");
            this.infiniteGridShader = new Shader("shaders/infinite_grid_vert.glsl", "shaders/infinite_grid_frag.glsl");
            this.lightPassShader = new Shader("shaders/light_pass_vert.glsl", "shaders/light_pass_frag.glsl");

            BuildGBufferQuad();
        }

        public override void Update(float deltaTime)
        {
            for (int i = 0; i < this.viewports.Count; i++)
            {
                this.viewports[i].mainCamera.Update(deltaTime);
                for (int c = 0; c < this.viewports[i].mainCamera.components.Count; c++)
                {
                    this.viewports[i].mainCamera.components[c].Update(deltaTime);
                }
            }

            // Temp : Updating topdown camera manually
            //var p = this.cameras[0].transform.GetPosition() + new Vector3(0, 50, 0);
            //this.cameras[1].view = Matrix4.LookAt(p, p - Vector3.UnitY, Vector3.UnitZ);
            //this.cameras[1].transform.SetPosition(p);
        }

        public override void Render(int frameBuffer)
        {
            if (!this.rendering) return;

            GL.ClearColor(0.1f, 0.1f, 0.15f, 1.0f);
            GL.Clear(ClearBufferMask.ColorBufferBit | ClearBufferMask.DepthBufferBit);
            GL.PolygonMode(MaterialFace.FrontAndBack, PolygonMode.Fill);

            // 1. geometry pass: render scene's geometry/color data into gbuffer
            // -----------------------------------------------------------------
            GL.BindFramebuffer(FramebufferTarget.Framebuffer, this.gBuffer);
            GL.Clear(ClearBufferMask.ColorBufferBit | ClearBufferMask.DepthBufferBit);

            if (this.shadingOverride == ShadingOverride.ShadedAndWireframe)
            {
                RenderShaded();
                RenderWireframe(true);
            }
            else if (this.shadingOverride == ShadingOverride.Wireframe)
            {
                RenderWireframe(false);
            }
            else if (this.shadingOverride == ShadingOverride.Shaded)
            {
                RenderShaded();
            }
            else if (this.shadingOverride == ShadingOverride.Unshaded)
            {
                RenderUnshaded();
            }
            else if (this.shadingOverride == ShadingOverride.Normals)
            {
                RenderNormals();
            }

            RenderGizmos();

            GL.BindFramebuffer(FramebufferTarget.Framebuffer, frameBuffer);

            GL.Viewport(this.x, this.y, this.width, this.height);


            // 2. lighting pass: calculate lighting by iterating over a screen filled quad pixel-by-pixel using the gbuffer's content.
            // -----------------------------------------------------------------------------------------------------------------------
            GL.Clear(ClearBufferMask.ColorBufferBit | ClearBufferMask.DepthBufferBit);
            GL.PolygonMode(MaterialFace.FrontAndBack, PolygonMode.Fill);
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
            this.viewports?[0].mainCamera.SetToShader(this.lightPassShader);

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

        private void RenderGizmos()
        {
            // Render Gizmos
            for (int i = 0; i < this.gizmos?.Count; i++)
            {
                if (this.gizmos[i] == null) continue;

                if (this.gizmos[i].visible == false) continue;

                if (this.gizmos[i] is Objects.Gizmos.CameraGizmo)
                {
                    var camGizmo = this.gizmos[i] as Objects.Gizmos.CameraGizmo;
                    if (camGizmo != null)
                        if (camGizmo.camera == this.viewports[0].mainCamera)
                            continue;
                }

                this.lineShader.Use();
                this.viewports[0].mainCamera.SetToShader(this.lineShader);
                this.gizmos[i].Render(Matrix4.Identity, this.lineShader);
            }
        }

        void RenderWireframe(bool smooth)
        {
            GL.PolygonMode(MaterialFace.FrontAndBack, PolygonMode.Line);
            GL.LineWidth(1.0f);

            if (smooth)
                GL.Enable(EnableCap.LineSmooth);
            else
                GL.Disable(EnableCap.LineSmooth);

            GL.Enable(EnableCap.Blend);
            GL.Enable(EnableCap.DepthTest);
            GL.BlendFunc(BlendingFactor.SrcAlpha, BlendingFactor.OneMinusSrcAlpha);

            //GL.Viewport(0, 0, this.width / 2, this.height);
            RenderInternal(this.wireframeShader, this.viewports[0].mainCamera);

            //GL.Viewport(this.width / 2, 0, this.width / 2, this.height);
            //RenderInternal(this.wireframeShader, this.cameras[1]);
        }

        void RenderNormals()
        {
            GL.PolygonMode(MaterialFace.FrontAndBack, PolygonMode.Fill);
            GL.Disable(EnableCap.Blend);
            GL.Enable(EnableCap.DepthTest);

            this.normalShader.SetColor4("lightColor", Vector4.Zero);
            this.normalShader.SetColor4("ambientColor", Vector4.One);

            RenderInternal(this.normalShader, this.viewports[0].mainCamera);
        }

        void RenderShaded()
        {
            GL.PolygonMode(MaterialFace.FrontAndBack, PolygonMode.Fill);
            GL.Disable(EnableCap.Blend);
            GL.Enable(EnableCap.DepthTest);

            // TODO : move this stuff to final (deferred)
            //for (int i = 0; i < this.lights.Count; i++)
            //{
                //this.lights[i].ApplyToShader(this.modelShader);
            //}

            this.modelShader.SetInt("diffuseMap0", 0);
            this.modelShader.SetInt("normalMap0", 1);
            this.modelShader.SetInt("diffuseMap1", 2);
            this.modelShader.SetInt("normalMap1", 3);
            this.modelShader.SetInt("diffuseMap2", 4);
            this.modelShader.SetInt("normalMap2", 5);
            this.modelShader.SetInt("diffuseMap3", 6);
            this.modelShader.SetInt("normalMap3", 7);

            RenderInternal(this.modelShader, this.viewports[0].mainCamera);
        }

        void RenderUnshaded()
        {
            GL.PolygonMode(MaterialFace.FrontAndBack, PolygonMode.Fill);
            GL.Disable(EnableCap.Blend);
            GL.Enable(EnableCap.DepthTest);

            this.modelShader.SetColor4("lightColor", Vector4.Zero);
            this.modelShader.SetColor4("ambientColor", Vector4.Zero);

            this.modelShader.SetInt("diffuseMap0", 0);
            this.modelShader.SetInt("normalMap0", 1);
            this.modelShader.SetInt("diffuseMap1", 2);
            this.modelShader.SetInt("normalMap1", 3);
            this.modelShader.SetInt("diffuseMap2", 4);
            this.modelShader.SetInt("normalMap2", 5);
            this.modelShader.SetInt("diffuseMap3", 6);
            this.modelShader.SetInt("normalMap3", 7);

            RenderInternal(this.modelShader, this.viewports[0].mainCamera);
        }

        void RenderInternal(Shader shader, Camera camera)
        {
            // camera/view transformation
            shader.Use();
            camera.SetToShader(shader);
            shader.SetVec3("objectColor", 1.0f, 1.0f, 1.0f);

            if (this.objects == null) return;

            for (int i = 0; i < this.objects.Count; i++)
            {
                if (this.objects[i] is Objects.M3Model)
                {
                    // pass model matrix
                    Matrix4 model = this.objects[i].transform.GetMatrix();
                    this.objects[i].Render(model, shader);
                }
            }
        }
    }
}
