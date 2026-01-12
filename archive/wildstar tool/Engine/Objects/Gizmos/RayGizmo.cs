using OpenTK.Graphics.OpenGL4;
using MathUtils;

namespace ProjectWS.Engine.Objects.Gizmos
{
    public class RayGizmo : Gizmo
    {
        int _vertexArrayObject;
        bool isBuilt;
        int[] indices;
        Vector4 color;

        public RayGizmo(Vector4 color) => this.color = color;

        public override void Build()
        {
            if (this.isBuilt) return;

            var vertices = new Vector3[]
            {
                new Vector3(0, 0, 0),
                new Vector3(0, 0, 1),
            };

            indices = new int[]
            {
                0,1
            };

            int _vertexBufferObject = GL.GenBuffer();
            GL.BindBuffer(BufferTarget.ArrayBuffer, _vertexBufferObject);
            GL.BufferData(BufferTarget.ArrayBuffer, vertices.Length * 3 * 4, vertices, BufferUsageHint.StaticDraw);

            _vertexArrayObject = GL.GenVertexArray();
            GL.BindVertexArray(_vertexArrayObject);

            GL.EnableVertexAttribArray(0);
            GL.VertexAttribPointer(0, 3, VertexAttribPointerType.Float, false, 12, 0);

            int _elementBufferObject = GL.GenBuffer();
            GL.BindBuffer(BufferTarget.ElementArrayBuffer, _elementBufferObject);
            GL.BufferData(BufferTarget.ElementArrayBuffer, indices.Length * 4, indices, BufferUsageHint.StaticDraw);

            GL.BindVertexArray(0);

            this.isBuilt = true;
        }

        public override void Render(Matrix4 model, Shader shader)
        {
            if (!this.isBuilt) return;
            var mat = model * transform.GetMatrix();
            shader.SetMat4("model", ref mat);
            shader.SetColor4("lineColor", this.color);

            GL.BindVertexArray(_vertexArrayObject);
            GL.DrawElements(BeginMode.Lines, indices.Length, DrawElementsType.UnsignedInt, 0);
        }
    }
}
