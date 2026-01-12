using MathUtils;
using OpenTK.Graphics.OpenGL4;
using System;
using System.Collections.Concurrent;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using static ProjectWS.Engine.Rendering.TextRenderer;
using SixLabors.ImageSharp;
using SixLabors.ImageSharp.PixelFormats;
using OpenTK.Compute.OpenCL;
using System.Runtime.InteropServices;
using SixLabors.ImageSharp.Advanced;
using System.Reflection.Emit;

namespace ProjectWS.Engine.Rendering
{
    public class IconRenderer
    {
        readonly ConcurrentQueue<Icon3D>? iconRenderQueue;
        int drawCalls;
        int quadVBO;
        int quadVAO;
        int texObj;
        bool init = false;

        public struct Icon3D
        {
            public enum Type
            {
                Dot,
                Light,
            }

            public Type type { get; set; }
            public Vector3 position { get; set; }
            public Vector4 color { get; set; }
        }

        public IconRenderer()
        {
            this.iconRenderQueue = new ConcurrentQueue<Icon3D>();
            this.drawCalls = 0;
        }

        public void Initialize()
        {
            if (init) return;

            using (Image<Rgba32> image = Image.Load<Rgba32>("Resources/Textures/LightBulbIcon.png"))
            {
                var _IMemoryGroup = image.GetPixelMemoryGroup();
                var _MemoryGroup = _IMemoryGroup.ToArray()[0];
                var PixelData = MemoryMarshal.AsBytes(_MemoryGroup.Span).ToArray();

                // create icon texture
                this.texObj = GL.GenTexture();
                GL.BindTexture(TextureTarget.Texture2D, texObj);
                GL.TexImage2D(TextureTarget.Texture2D, 0, PixelInternalFormat.Rgba, image.Width, image.Height, 0, OpenTK.Graphics.OpenGL4.PixelFormat.Rgba, PixelType.UnsignedByte, PixelData);

                // set texture parameters
                GL.TextureParameter(texObj, TextureParameterName.TextureMinFilter, (int)TextureMinFilter.Linear);
                GL.TextureParameter(texObj, TextureParameterName.TextureMagFilter, (int)TextureMagFilter.Linear);
                GL.TextureParameter(texObj, TextureParameterName.TextureWrapS, (int)TextureWrapMode.ClampToEdge);
                GL.TextureParameter(texObj, TextureParameterName.TextureWrapT, (int)TextureWrapMode.ClampToEdge);
            }

            float[] quadVertices = new float[]{
                    // positions        // texture Coords
                    -1.0f,  1.0f, 0.0f, 0.0f, 1.0f,
                    -1.0f, -1.0f, 0.0f, 0.0f, 0.0f,
                     1.0f,  1.0f, 0.0f, 1.0f, 1.0f,
                     1.0f, -1.0f, 0.0f, 1.0f, 0.0f,
                };
            // setup plane VAO
            this.quadVAO = GL.GenVertexArray();
            this.quadVBO = GL.GenBuffer();
            GL.BindVertexArray(this.quadVAO);
            GL.BindBuffer(BufferTarget.ArrayBuffer, this.quadVBO);
            GL.BufferData(BufferTarget.ArrayBuffer, 4 * quadVertices.Length, quadVertices, BufferUsageHint.StaticDraw);
            GL.EnableVertexAttribArray(0);
            GL.VertexAttribPointer(0, 3, VertexAttribPointerType.Float, false, 5 * sizeof(float), 0);
            GL.EnableVertexAttribArray(1);
            GL.VertexAttribPointer(1, 2, VertexAttribPointerType.Float, false, 5 * sizeof(float), 3 * 4);

            init = true;
        }

        public void Render(Renderer renderer, Viewport viewport)
        {
            this.drawCalls = 0;
            RenderIcons(renderer, viewport);
        }

        public void RenderIcons(Renderer renderer, Viewport vp)
        {
            if (this.iconRenderQueue?.Count > 0)
            {
                renderer.iconShader.Use();
                GL.Enable(EnableCap.Blend);
                GL.BlendFunc(BlendingFactor.SrcAlpha, BlendingFactor.OneMinusSrcAlpha);
                GL.Disable(EnableCap.DepthTest);
                renderer.iconShader.SetFloat("viewportAspect", vp.aspect);
                renderer.iconShader.SetVec3("cameraPos", vp.mainCamera.transform.GetPosition());
                vp.mainCamera.SetToShader(renderer.iconShader);
            }

            for (int i = 0; i < this.iconRenderQueue?.Count; i++)
            {
                if (this.iconRenderQueue.TryDequeue(out Icon3D icon))
                {
                    this.drawCalls++;

                    renderer.iconShader.SetVec3("vertexPosition_worldspace", icon.position);
                    
                    GL.ActiveTexture(TextureUnit.Texture0);

                    // Render glyph texture over quad
                    GL.BindTexture(TextureTarget.Texture2D, this.texObj);

                    // Render quad
                    GL.BindVertexArray(this.quadVAO);
                    GL.DrawArrays(PrimitiveType.TriangleStrip, 0, 4);
                    GL.BindVertexArray(0);
                }
            }
        }

        /// <summary>
        /// Renderd an icon in 3D space using immediate mode
        /// </summary>
        /// <param name="type">The icon type, which determines the sprite to use</param>
        /// <param name="position">World space position</param>
        /// <param name="color">Text color</param>
        /// <param name="shadow">Enable text shadow effect</param>
        public void DrawIcon3D(Icon3D.Type type, Vector3 position, Vector4 color)
        {
            this.iconRenderQueue?.Enqueue(new Icon3D { type = type, position = position, color = color });
        }

        unsafe void SetMat(ref Matrix4 mat)
        {
            fixed (float* value = &mat.Row0.X)
            {
                GL.UniformMatrix4(0, 1, false, value);
            }
        }
    }
}
