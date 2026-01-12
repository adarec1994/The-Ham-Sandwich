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
    public class BoxGizmo : Gizmo
    {
        int vao;
        int[]? indices;
        bool isBuilt;
        Vector4 color;

        public BoxGizmo(Vector4 color) => this.color = color;

        public override void Build()
        {
            if (this.isBuilt) return;

            float cubeSize = 1.0f;
            float ch = cubeSize / 2.0f;

            var vertices = new Vector3[]
            {
                new Vector3(-ch, -ch, -ch),
                new Vector3(ch, -ch, -ch),
                new Vector3(ch, ch, -ch),
                new Vector3(-ch, ch, -ch),
                new Vector3(-ch, -ch, ch),
                new Vector3(ch, -ch, ch),
                new Vector3(ch, ch, ch),
                new Vector3(-ch, ch, ch)
            };

            this.indices = new int[]
            {
                0,1, 1,2, 2,3, 3,0, 4,5, 5,6, 6,7, 7,4, 0,4, 1,5, 2,6, 3,7
            };

            int _vertexBufferObject = GL.GenBuffer();
            GL.BindBuffer(BufferTarget.ArrayBuffer, _vertexBufferObject);
            GL.BufferData(BufferTarget.ArrayBuffer, vertices.Length * 3 * 4, vertices, BufferUsageHint.StaticDraw);

            this.vao = GL.GenVertexArray();
            GL.BindVertexArray(this.vao);

            GL.EnableVertexAttribArray(0);
            GL.VertexAttribPointer(0, 3, VertexAttribPointerType.Float, false, 12, 0);

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

            var mat = model * this.transform.GetMatrix();

            shader.SetMat4("model", ref mat);
            shader.SetColor4("lineColor", this.color);

            GL.BindVertexArray(this.vao);
            GL.DrawElements(BeginMode.Lines, this.indices.Length, DrawElementsType.UnsignedInt, 0);
        }
    }
}
