using MathUtils;
using OpenTK.Graphics.OpenGL4;
using ProjectWS.Engine.World;
using System.Runtime.InteropServices;

namespace ProjectWS.Engine.Mesh
{
    public class TerrainMesh : Mesh
    {
        const int VERTEXCOUNT = 361;
        const int VERTEXSIZE = 48;

        SubChunk subChunk;

        [StructLayout(LayoutKind.Sequential)]
        public struct TerrainVertex
        {
            public Vector3 position;
            public Vector3 normal;
            public Vector4 tangent;
            public Vector2 uv;
        }

        public TerrainVertex[]? vertices;

        public float minHeight;
        public float maxHeight;

        public uint[]? indexData;
        public bool isBuilt;
        public int _vertexArrayObject;
        public int _vertexBufferObject;

        public TerrainMesh(ushort[] heightMap, SubChunk subChunk)
        {
            if (heightMap == null) return;
            this.subChunk = subChunk;
            this.minHeight = float.MaxValue;
            this.maxHeight = float.MinValue;

            if (heightMap.Length == VERTEXCOUNT)
            {
                // LoD 0 //
                this.vertices = new TerrainVertex[VERTEXCOUNT];

                int index = 0;
                for (int y = -1; y < 18; ++y)
                {
                    for (int x = -1; x < 18; ++x)
                    {
                        float h = ((heightMap[(y + 1) * 19 + x + 1] & 0x7FFF) * 0.12500381f) - 2048.0f;

                        // Calc minmax
                        if (h < this.minHeight)
                            this.minHeight = h;
                        if (h > this.maxHeight)
                            this.maxHeight = h;

                        // Positions //
                        this.vertices[index].position = new Vector3(x * 2, h, y * 2);

                        // Normals //
                        if (y > 0 && x > 0 && y <= 17 && y <= 17)
                        {
                            Vector3 tl = this.vertices[(y - 1) * 19 + x - 1].position;
                            Vector3 tr = this.vertices[(y - 1) * 19 + x + 1].position;
                            Vector3 br = this.vertices[(y + 1) * 19 + x + 1].position;
                            Vector3 bl = this.vertices[(y + 1) * 19 + x - 1].position;
                            Vector3 v = this.vertices[y * 19 + x].position;
                            Vector3 P1 = new Vector3(tl.X, tl.Y, tl.Z);
                            Vector3 P2 = new Vector3(tr.X, tr.Y, tr.Z);
                            Vector3 P3 = new Vector3(br.X, br.Y, br.Z);
                            Vector3 P4 = new Vector3(bl.X, bl.Y, bl.Z);
                            Vector3 vert = new Vector3(v.X, v.Y, v.Z);
                            Vector3 N1 = Vector3.Cross((P2 - vert), (P1 - vert));
                            Vector3 N2 = Vector3.Cross((P3 - vert), (P2 - vert));
                            Vector3 N3 = Vector3.Cross((P4 - vert), (P3 - vert));
                            Vector3 N4 = Vector3.Cross((P1 - vert), (P4 - vert));
                            Vector3 norm = Vector3.Normalize(N1 + N2 + N3 + N4);
                            this.vertices[y * 19 + x].normal = norm;
                        }
                        index++;
                    }
                }
                //this.vertices = Trim19x19to17x17(this.vertices);

                // Triangles //
                this.indexData = new uint[16 * 16 * 2 * 3];
                int triOffset = 0;
                for (int strip = 1; strip < 17; strip++)
                {
                    //   case Up-Left   //
                    for (int t = 1; t < 17; t++)
                    {
                        this.indexData[triOffset + 0] = (uint)(t + strip * 19);
                        this.indexData[triOffset + 1] = (uint)(t + 1 + strip * 19);
                        this.indexData[triOffset + 2] = (uint)(t + (strip + 1) * 19);
                        triOffset = triOffset + 3;
                    }
                    //   case Down-Right   //
                    for (int t = 1; t < 17; t++)
                    {
                        this.indexData[triOffset + 0] = (uint)(t + 1 + strip * 19);
                        this.indexData[triOffset + 1] = (uint)(t + 1 + (strip + 1) * 19);
                        this.indexData[triOffset + 2] = (uint)(t + (strip + 1) * 19);
                        triOffset = triOffset + 3;
                    }
                }

                // UVs //
                var offs = 1.0f / 16.0f;
                for (int u = 0; u < 19; u++)
                {
                    for (int v = 0; v < 19; v++)
                    {
                        this.vertices[u + v * 19].uv = new Vector2(u / 16.0f - offs, v / 16.0f - offs);
                    }
                }

                // Tangents //
                Vector3[] tan1 = new Vector3[this.vertices.Length];
                Vector3[] tan2 = new Vector3[this.vertices.Length];
                for (long a = 0; a < this.indexData.Length; a += 3)
                {
                    long i1 = this.indexData[a + 0];
                    long i2 = this.indexData[a + 1];
                    long i3 = this.indexData[a + 2];

                    Vector3 v1 = this.vertices[i1].position;
                    Vector3 v2 = this.vertices[i2].position;
                    Vector3 v3 = this.vertices[i3].position;

                    Vector2 w1 = this.vertices[i1].uv;
                    Vector2 w2 = this.vertices[i2].uv;
                    Vector2 w3 = this.vertices[i3].uv;

                    float x1 = v2.X - v1.X;
                    float x2 = v3.X - v1.X;
                    float y1 = v2.Y - v1.Y;
                    float y2 = v3.Y - v1.Y;
                    float z1 = v2.Z - v1.Z;
                    float z2 = v3.Z - v1.Z;

                    float s1 = w2.X - w1.X;
                    float s2 = w3.X - w1.X;
                    float t1 = w2.Y - w1.Y;
                    float t2 = w3.Y - w1.Y;

                    float r = 1.0f / (s1 * t2 - s2 * t1);

                    Vector3 sdir = new Vector3((t2 * x1 - t1 * x2) * r, (t2 * y1 - t1 * y2) * r, (t2 * z1 - t1 * z2) * r);
                    Vector3 tdir = new Vector3((s1 * x2 - s2 * x1) * r, (s1 * y2 - s2 * y1) * r, (s1 * z2 - s2 * z1) * r);

                    tan1[i1] += sdir;
                    tan1[i2] += sdir;
                    tan1[i3] += sdir;

                    tan2[i1] += tdir;
                    tan2[i2] += tdir;
                    tan2[i3] += tdir;
                }
                for (long a = 0; a < this.vertices.Length; ++a)
                {
                    Vector3 n = this.vertices[a].normal;
                    Vector3 t = tan1[a];

                    Vector3 tmp = (t - n * Vector3.Dot(n, t)).Normalized();
                    this.vertices[a].tangent = new Vector4(tmp.X, tmp.Y, tmp.Z, 1.0f); // or -1.0? if uv is flipped?
                }
            }
            else
            {
                // LoD 1 //
            }
        }

        public override void Build()
        {
            // Some subchunks don't exist
            if (this.vertices == null) return;

            this._vertexBufferObject = GL.GenBuffer();
            GL.BindBuffer(BufferTarget.ArrayBuffer, this._vertexBufferObject);
            GL.BufferData(BufferTarget.ArrayBuffer, this.vertices.Length * VERTEXSIZE, this.vertices, BufferUsageHint.DynamicDraw);

            this._vertexArrayObject = GL.GenVertexArray();
            GL.BindVertexArray(this._vertexArrayObject);

            GL.EnableVertexAttribArray(0);
            GL.EnableVertexAttribArray(1);
            GL.EnableVertexAttribArray(2);
            GL.EnableVertexAttribArray(3);

            GL.VertexAttribPointer(0, 3, VertexAttribPointerType.Float, false, VERTEXSIZE, 0);
            GL.VertexAttribPointer(1, 3, VertexAttribPointerType.Float, true, VERTEXSIZE, 12);
            GL.VertexAttribPointer(2, 4, VertexAttribPointerType.Float, true, VERTEXSIZE, 24);
            GL.VertexAttribPointer(3, 2, VertexAttribPointerType.Float, false, VERTEXSIZE, 40);

            int _elementBufferObject = GL.GenBuffer();
            GL.BindBuffer(BufferTarget.ElementArrayBuffer, _elementBufferObject);
            GL.BufferData(BufferTarget.ElementArrayBuffer, this.indexData.Length * 4, this.indexData, BufferUsageHint.StaticDraw);

            GL.BindVertexArray(0);

            this.isBuilt = true;
        }

        public void ReBuild()
        {
            this.subChunk.chunk.modified = true;

            int index = 0;
            for (int y = -1; y < 18; ++y)
            {
                for (int x = -1; x < 18; ++x)
                {
                    float h = this.vertices[index].position.Y;

                    // Update height map
                    this.subChunk.subArea.heightMap[(y + 1) * 19 + x + 1] = (ushort)((h + 2048.0f) / 0.12500381f);

                    // Recalculate minmax
                    if (h < this.minHeight)
                        this.minHeight = h;
                    if (h > this.maxHeight)
                        this.maxHeight = h;

                    // Recalculate normal
                    if (y > 0 && x > 0 && y <= 17 && y <= 17)
                    {
                        Vector3 tl = this.vertices[(y - 1) * 19 + x - 1].position;
                        Vector3 tr = this.vertices[(y - 1) * 19 + x + 1].position;
                        Vector3 br = this.vertices[(y + 1) * 19 + x + 1].position;
                        Vector3 bl = this.vertices[(y + 1) * 19 + x - 1].position;
                        Vector3 v = this.vertices[y * 19 + x].position;
                        Vector3 P1 = new Vector3(tl.X, tl.Y, tl.Z);
                        Vector3 P2 = new Vector3(tr.X, tr.Y, tr.Z);
                        Vector3 P3 = new Vector3(br.X, br.Y, br.Z);
                        Vector3 P4 = new Vector3(bl.X, bl.Y, bl.Z);
                        Vector3 vert = new Vector3(v.X, v.Y, v.Z);
                        Vector3 N1 = Vector3.Cross((P2 - vert), (P1 - vert));
                        Vector3 N2 = Vector3.Cross((P3 - vert), (P2 - vert));
                        Vector3 N3 = Vector3.Cross((P4 - vert), (P3 - vert));
                        Vector3 N4 = Vector3.Cross((P1 - vert), (P4 - vert));
                        Vector3 norm = Vector3.Normalize(N1 + N2 + N3 + N4);
                        this.vertices[y * 19 + x].normal = norm;
                    }
                    index++;
                }
            }

            // Update bounds
            Vector3 subchunkRelativePosition = new Vector3(this.subChunk.X * 32f, 0, this.subChunk.Y * 32f);
            this.subChunk.centerPosition = this.subChunk.chunk.worldCoords + subchunkRelativePosition + new Vector3(16f, ((this.maxHeight - this.minHeight) / 2f) + this.minHeight, 16f);
            this.subChunk.AABB = new AABB(this.subChunk.centerPosition, new Vector3(32f, this.maxHeight - this.minHeight, 32f));            // Exact bounds
            this.subChunk.cullingAABB = new AABB(this.subChunk.centerPosition, new Vector3(64f, (this.maxHeight - this.minHeight) * 2, 64f));

            // Upload to GPU
            // https://stackoverflow.com/questions/15821969/what-is-the-proper-way-to-modify-opengl-vertex-buffer
            GL.BindBuffer(BufferTarget.ArrayBuffer, this._vertexBufferObject);
            GL.BufferData(BufferTarget.ArrayBuffer, this.vertices.Length * VERTEXSIZE, this.vertices, BufferUsageHint.DynamicDraw);
        }

        public override void Draw()
        {
            if (!this.isBuilt) return;

            GL.BindVertexArray(_vertexArrayObject);
            GL.DrawElements(BeginMode.Triangles, this.indexData.Length, DrawElementsType.UnsignedInt, 0);
        }

        public override void DrawInstanced()
        {
            throw new NotImplementedException();
        }

        TerrainVertex[] Trim19x19to17x17(TerrainVertex[] vector3in)
        {
            TerrainVertex[] vector3out = new TerrainVertex[17 * 17];
            for (int x = 1; x <= 17; x++)
            {
                for (int y = 1; y <= 17; y++)
                {
                    vector3out[(x - 1) + (y - 1) * 17] = vector3in[x + y * 19];
                }
            }
            return vector3out;
        }

        public override bool MeshIntersectsRay(Ray ray, Vector3 wPos, Quaternion wRot, Vector3 wScale, ref Vector3[] points)
        {
            bool haveIntersection = false;
            int triIdx = 0;

            for (int i = 0; i < this.indexData.Length; i += 3)
            {
                uint i0 = this.indexData[i];
                uint i1 = this.indexData[i + 1];
                uint i2 = this.indexData[i + 2];

                var v0 = this.vertices[i0].position + wPos;
                var v1 = this.vertices[i1].position + wPos;
                var v2 = this.vertices[i2].position + wPos;

                if (this.TriangleIntersectsRay(ray, v0, v1, v2, out points[triIdx]))
                {
                    triIdx++;
                    haveIntersection = true;

                    if (triIdx >= 4)
                        break;
                }
            }

            return haveIntersection;
        }

        internal void Unload()
        {
            this.isBuilt = false;

            GL.DeleteVertexArray(this._vertexArrayObject);
            GL.DeleteBuffer(this._vertexBufferObject);

            this.vertices = null;
            this.indexData = null;
        }
    }
}
