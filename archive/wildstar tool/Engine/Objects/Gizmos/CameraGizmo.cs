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
    public class CameraGizmo : Gizmo
    {
        public Camera camera;
        int _vertexArrayObject;
        bool isBuilt;
        int[] indices;

        public CameraGizmo(Camera camera = null)
        {
            this.camera = camera;
        }

        public override void Build()
        {

            float X0 = camera.farDistance * (float)Math.Cos(camera.fov / 2 - 1.5708f);
            float Y0 = camera.farDistance * (float)Math.Sin(camera.fov / 2 - 1.5708f);
            float X1 = camera.farDistance * (float)Math.Cos(-camera.fov / 2 - 1.5708f);
            float Y1 = camera.farDistance * (float)Math.Sin(-camera.fov / 2 - 1.5708f);

            float X2 = camera.nearDistance * (float)Math.Cos(camera.fov / 2 - 1.5708f);
            float Y2 = camera.nearDistance * (float)Math.Sin(camera.fov / 2 - 1.5708f);
            float X3 = camera.nearDistance * (float)Math.Cos(-camera.fov / 2 - 1.5708f);
            float Y3 = camera.nearDistance * (float)Math.Sin(-camera.fov / 2 - 1.5708f);

            float d0 = Vector2.Distance(new Vector2(X0, Y0), new Vector2(X1, Y1));
            float d1 = d0 / this.camera.aspectRatio;

            float d2 = Vector2.Distance(new Vector2(X2, Y2), new Vector2(X3, Y3));
            float d3 = d2 / this.camera.aspectRatio;

            float cubeSize = 1.0f;
            float ch = cubeSize / 2.0f;

            var line = new Vector3[]
            {
                // Zero
                new Vector3(0, 0, 0),

                // Far Verts
                new Vector3(X0, d1 / 2, Y0),
                new Vector3(X1, d1 / 2, Y1),
                new Vector3(X0, -d1 / 2, Y0),
                new Vector3(X1, -d1 / 2, Y1),

                // Near Verts
                new Vector3(X2, d3 / 2, Y2),
                new Vector3(X3, d3 / 2, Y3),
                new Vector3(X2, -d3 / 2, Y2),
                new Vector3(X3, -d3 / 2, Y3),

                // Cube
                new Vector3(-ch, -ch, -ch),
                new Vector3(ch, -ch, -ch),
                new Vector3(ch, ch, -ch),
                new Vector3(-ch, ch, -ch),
                new Vector3(-ch, -ch, ch),
                new Vector3(ch, -ch, ch),
                new Vector3(ch, ch, ch),
                new Vector3(-ch, ch, ch)
            };

            var frustumIndices = new int[]
            {
                5, 1, 6, 2, 7, 3, 8, 4, // angle lines (from near square)
                //0, 1, 0, 2, 0, 3, 0, 4, // angle lines (from point)
                1, 3, 3, 4, 4, 2, 2, 1, // far square
                5, 7, 7, 8, 8, 6, 6, 5, // near square
            };

            var cubeIndices = new int[]
            {
                0,1, 1,2, 2,3, 3,0, 4,5, 5,6, 6,7, 7,4, 0,4, 1,5, 2,6, 3,7
            };

            this.indices = new int[frustumIndices.Length + cubeIndices.Length];
            Array.Copy(frustumIndices, this.indices, frustumIndices.Length);
            for (int i = 0; i < cubeIndices.Length; i++)
            {
                this.indices[i + frustumIndices.Length] = cubeIndices[i] + 9;
            }

            int _vertexBufferObject = GL.GenBuffer();
            GL.BindBuffer(BufferTarget.ArrayBuffer, _vertexBufferObject);
            GL.BufferData(BufferTarget.ArrayBuffer, line.Length * 3 * 4, line, BufferUsageHint.StaticDraw);

            _vertexArrayObject = GL.GenVertexArray();
            GL.BindVertexArray(_vertexArrayObject);

            GL.EnableVertexAttribArray(0);
            GL.VertexAttribPointer(0, 3, VertexAttribPointerType.Float, false, 0, 0);

            int _elementBufferObject = GL.GenBuffer();
            GL.BindBuffer(BufferTarget.ElementArrayBuffer, _elementBufferObject);
            GL.BufferData(BufferTarget.ElementArrayBuffer, indices.Length * 4, indices, BufferUsageHint.StaticDraw);

            GL.BindVertexArray(0);

            this.isBuilt = true;
        }

        public override void Render(Matrix4 model, Shader shader)
        {
            if (!this.isBuilt) return;

            //var mat = camera.transform.GetMatrix();
            var mat = Matrix4.Identity;
            var rot = camera.transform.GetRotation();
            rot.Invert();
            mat *= Matrix4.CreateFromQuaternion(rot);
            mat *= Matrix4.CreateTranslation(camera.transform.GetPosition());
            shader.SetMat4("model", ref mat);
            shader.SetColor4("lineColor", Vector4.One);

            // Draw the line
            GL.BindVertexArray(_vertexArrayObject);
            GL.DrawElements(BeginMode.Lines, indices.Length, DrawElementsType.UnsignedInt, 0);
        }
    }
}
