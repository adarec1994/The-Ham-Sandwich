using MathUtils;
using OpenTK.Graphics.OpenGL4;

namespace ProjectWS.Engine.Mesh
{
    public class WaterMesh : Mesh
    {
        const int VERTEXSIZE = 104;

        public float minHeight;
        public float maxHeight;

        public bool isBuilt;
        public int _vertexArrayObject;
        public int _vertexBufferObject;
        private FileFormats.Area.Water water;

        public WaterMesh(FileFormats.Area.Water water)
        {
            this.water = water;
        }

        public override void Build()
        {
            // Some subchunks don't exist
            if (this.water.vertices == null || this.water.indexData == null) return;

            this._vertexBufferObject = GL.GenBuffer();
            GL.BindBuffer(BufferTarget.ArrayBuffer, this._vertexBufferObject);
            GL.BufferData(BufferTarget.ArrayBuffer, this.water.vertices.Length * VERTEXSIZE, this.water.vertices, BufferUsageHint.StaticDraw);

            this._vertexArrayObject = GL.GenVertexArray();
            GL.BindVertexArray(this._vertexArrayObject);

            GL.EnableVertexAttribArray(0);
            GL.EnableVertexAttribArray(1);
            GL.EnableVertexAttribArray(2);
            GL.EnableVertexAttribArray(3);
            GL.EnableVertexAttribArray(4);
            GL.EnableVertexAttribArray(5);
            GL.EnableVertexAttribArray(6);
            GL.EnableVertexAttribArray(7);
            GL.EnableVertexAttribArray(8);

            GL.VertexAttribPointer(0, 3, VertexAttribPointerType.Float, false, VERTEXSIZE, 0);      // position 3 * 4 = 12
            GL.VertexAttribPointer(1, 3, VertexAttribPointerType.Float, true, VERTEXSIZE, 12);      // normal 12 + 3 * 4 = 24
            GL.VertexAttribPointer(2, 4, VertexAttribPointerType.Float, true, VERTEXSIZE, 24);      // tangent 24 + 4 * 4 = 40
            GL.VertexAttribPointer(3, 4, VertexAttribPointerType.Float, true, VERTEXSIZE, 40);      // bitangent 40 + 4 * 4 = 56
            GL.VertexAttribPointer(4, 2, VertexAttribPointerType.Float, false, VERTEXSIZE, 56);     // uv 56 + 2 * 4 = 64
            GL.VertexAttribPointer(5, 4, VertexAttribPointerType.Float, false, VERTEXSIZE, 64);     // color 64 + 4 * 4 = 80
            GL.VertexAttribPointer(6, 1, VertexAttribPointerType.Float, false, VERTEXSIZE, 80);     // unk0 80 + 4 = 84
            GL.VertexAttribPointer(7, 1, VertexAttribPointerType.Int, false, VERTEXSIZE, 84);       // unk1 84 + 4 = 88
            GL.VertexAttribPointer(8, 4, VertexAttribPointerType.Float, false, VERTEXSIZE, 88);    // mask 88 + 4 * 4 = 104

            int _elementBufferObject = GL.GenBuffer();
            GL.BindBuffer(BufferTarget.ElementArrayBuffer, _elementBufferObject);
            GL.BufferData(BufferTarget.ElementArrayBuffer, this.water.indexData.Length * 4, this.water.indexData, BufferUsageHint.StaticDraw);

            GL.BindVertexArray(0);

            this.isBuilt = true;
        }

        public override void Draw()
        {
            if (!this.isBuilt) return;

            GL.BindVertexArray(this._vertexArrayObject);
            GL.DrawElements(BeginMode.Triangles, this.water.indexData.Length, DrawElementsType.UnsignedInt, 0);
        }

        public override void DrawInstanced()
        {
            throw new NotImplementedException();
        }

        public override bool MeshIntersectsRay(Ray ray, Vector3 wPos, Quaternion wRot, Vector3 wScale, ref Vector3[] points)
        {
            throw new NotImplementedException();
        }

        internal void Unload()
        {
            this.isBuilt = false;

            GL.DeleteVertexArray(this._vertexArrayObject);
            GL.DeleteBuffer(this._vertexBufferObject);
        }
    }
}
