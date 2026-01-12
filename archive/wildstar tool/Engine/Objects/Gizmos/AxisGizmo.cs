using OpenTK;
using OpenTK.Graphics.OpenGL4;
using MathUtils;
using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace ProjectWS.Engine.Objects.Gizmos
{
    public class AxisGizmo : Gizmo
    {
        int vao;
        int[]? indices;
        bool isBuilt;
        Vector4 color;

        public AxisGizmo() { }

        public override void Build()
        {
            if (this.isBuilt) return;

            float size = 1.0f;
            float ch = size;

            this.color = new Vector4(1.0f, 1.0f, 1.0f, 1.0f);

            var vertices = new LineVertexBuffer[]
            {
                new LineVertexBuffer(new Vector3 (0.0f, 0.0f, 0.0f), new Vector4(1.0f, 0.0f, 0.0f, 1.0f)),
                new LineVertexBuffer(new Vector3 (ch, 0.0f, 0.0f), new Vector4(1.0f, 0.0f, 0.0f, 1.0f)),

                new LineVertexBuffer(new Vector3 (0.0f, 0.0f, 0.0f), new Vector4(0.0f, 1.0f, 0.0f, 1.0f)),
                new LineVertexBuffer(new Vector3 (0.0f, ch, 0.0f), new Vector4(0.0f, 1.0f, 0.0f, 1.0f)),

                new LineVertexBuffer(new Vector3 (0.0f, 0.0f, 0.0f), new Vector4(0.0f, 0.0f, 1.0f, 1.0f)),
                new LineVertexBuffer(new Vector3 (0.0f, 0.0f, ch), new Vector4(0.0f, 0.0f, 1.0f, 1.0f)),
            };

            this.indices = new int[]
            {
                0,1, 2,3, 4,5
            };

            int _vertexBufferObject = GL.GenBuffer();
            GL.BindBuffer(BufferTarget.ArrayBuffer, _vertexBufferObject);
            GL.BufferData(BufferTarget.ArrayBuffer, vertices.Length * 28, vertices, BufferUsageHint.StaticDraw);

            this.vao = GL.GenVertexArray();
            GL.BindVertexArray(this.vao);

            // Position
            GL.VertexAttribPointer(0, 3, VertexAttribPointerType.Float, false, 28, 0);
            GL.EnableVertexAttribArray(0);

            // Color
            GL.VertexAttribPointer(1, 4, VertexAttribPointerType.Float, false, 28, 12);
            GL.EnableVertexAttribArray(1);

            int _elementBufferObject = GL.GenBuffer();
            GL.BindBuffer(BufferTarget.ElementArrayBuffer, _elementBufferObject);
            GL.BufferData(BufferTarget.ElementArrayBuffer, this.indices.Length * 4, this.indices, BufferUsageHint.StaticDraw);

            GL.BindVertexArray(0);

            this.isBuilt = true;
        }

        public override void Render(Matrix4 model, Shader shader)
        {
            if (!this.isBuilt) return;
            if (this.indices == null) return;

            GL.Disable(EnableCap.DepthTest);

            var mat = model * this.transform.GetMatrix();

            shader.SetMat4("model", ref mat);
            shader.SetColor4("lineColor", this.color);

            GL.BindVertexArray(this.vao);
            GL.DrawElements(BeginMode.Lines, this.indices.Length, DrawElementsType.UnsignedInt, 0);

            GL.Enable(EnableCap.DepthTest);
        }
    }
}
