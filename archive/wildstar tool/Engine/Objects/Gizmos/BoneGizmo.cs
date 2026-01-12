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
    public partial class BoneGizmo : Gizmo
    {
        int lineVAO;
        int sphVAO;
        int[] lineIndices;
        int[] sphIndices;
        bool isBuilt;
        Vector4 color;

        public BoneGizmo() { }

        public override void Build()
        {
            if (this.isBuilt) return;

            float size = 1.0f;
            float ch = size;

            this.color = new Vector4(1.0f, 1.0f, 1.0f, 1.0f);
            GenerateLine();
            GenerateCircles();

            this.isBuilt = true;
        }

        private void GenerateLine()
        {
            var vertices = new LineVertexBuffer[]
            {
                new LineVertexBuffer(new Vector3 (0.0f, 0.0f, 0.0f), new Vector4(1.0f, 0.0f, 0.0f, 1.0f)),
                new LineVertexBuffer(new Vector3 (0.0f, 0.0f, -1.0f), new Vector4(1.0f, 0.0f, 0.0f, 1.0f)),
            };

            this.lineIndices = new int[]
            {
                0,1
            };

            int _vertexBufferObject = GL.GenBuffer();
            GL.BindBuffer(BufferTarget.ArrayBuffer, _vertexBufferObject);
            GL.BufferData(BufferTarget.ArrayBuffer, vertices.Length * 28, vertices, BufferUsageHint.StaticDraw);

            this.lineVAO = GL.GenVertexArray();
            GL.BindVertexArray(this.lineVAO);

            // Position
            GL.VertexAttribPointer(0, 3, VertexAttribPointerType.Float, false, 28, 0);
            GL.EnableVertexAttribArray(0);

            // Color
            GL.VertexAttribPointer(1, 4, VertexAttribPointerType.Float, false, 28, 12);
            GL.EnableVertexAttribArray(1);

            int _elementBufferObject = GL.GenBuffer();
            GL.BindBuffer(BufferTarget.ElementArrayBuffer, _elementBufferObject);
            GL.BufferData(BufferTarget.ElementArrayBuffer, this.lineIndices.Length * 4, this.lineIndices, BufferUsageHint.StaticDraw);

            GL.BindVertexArray(0);
        }
        private void GenerateCircles()
        {
            int numSegments = 16;
            float radius = 0.03f;

            var vertices = GenerateWireSphereVertices(radius, numSegments, new Vector4(1.0f, 1.0f, 1.0f, 1.0f));

            this.sphIndices = GenerateWireSphereIndices(numSegments);

            int _vertexBufferObject = GL.GenBuffer();
            GL.BindBuffer(BufferTarget.ArrayBuffer, _vertexBufferObject);
            GL.BufferData(BufferTarget.ArrayBuffer, vertices.Length * 28, vertices, BufferUsageHint.StaticDraw);

            this.sphVAO = GL.GenVertexArray();
            GL.BindVertexArray(this.sphVAO);

            // Position
            GL.VertexAttribPointer(0, 3, VertexAttribPointerType.Float, false, 28, 0);
            GL.EnableVertexAttribArray(0);

            // Color
            GL.VertexAttribPointer(1, 4, VertexAttribPointerType.Float, false, 28, 12);
            GL.EnableVertexAttribArray(1);

            int _elementBufferObject = GL.GenBuffer();
            GL.BindBuffer(BufferTarget.ElementArrayBuffer, _elementBufferObject);
            GL.BufferData(BufferTarget.ElementArrayBuffer, this.sphIndices.Length * 4, this.sphIndices, BufferUsageHint.StaticDraw);

            GL.BindVertexArray(0);
        }

        public override void Render(Matrix4 model, Shader shader)
        {
            if (!this.isBuilt) return;
            if (this.lineIndices == null) return;

            var mat = model * this.transform.GetMatrix();

            GL.Disable(EnableCap.DepthTest);

            shader.SetMat4("model", ref mat);
            shader.SetColor4("lineColor", this.color);

            GL.BindVertexArray(this.lineVAO);
            GL.DrawElements(BeginMode.Lines, this.lineIndices.Length, DrawElementsType.UnsignedInt, 0);

            var curScale = this.transform.GetScale();
            mat = Matrix4.CreateScale(curScale).Inverted() * mat;
            
            shader.SetMat4("model", ref mat);
            GL.BindVertexArray(this.sphVAO);
            GL.DrawElements(BeginMode.Lines, this.sphIndices.Length, DrawElementsType.UnsignedInt, 0);

            GL.Enable(EnableCap.DepthTest);
        }
    }
}
