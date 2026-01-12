using MathUtils;
using OpenTK.Graphics.OpenGL4;
using ProjectWS.FileFormats.Extensions;

namespace ProjectWS.Engine.Mesh
{
    public class M3Mesh : Mesh
    {
        public FileFormats.M3.Submesh? data;
        int vertexBlockSizeInBytes;
        byte[]? vertexBlockFieldPositions;
        FileFormats.M3.Geometry.VertexBlockFlags vertexBlockFlags;
        FileFormats.M3.Geometry.VertexFieldType[]? vertexFieldTypes;

        public bool isBuilt;
        public bool positionCompressed;
        public bool renderable;
        public int VAO;
        public int VBO;
        public int EBO;

        public uint[]? indexData;
        public byte[]? vertexData;
        public Vector3[]? vertexPositions;       // Required for the mouse picking algorithm
        public Matrix4[]? instances;
        int instanceBuffer;

        public M3Mesh(FileFormats.M3.Submesh submesh, int vertexBlockSizeInBytes, byte[] vertexBlockFieldPositions, FileFormats.M3.Geometry.VertexBlockFlags vertexBlockFlags, FileFormats.M3.Geometry.VertexFieldType[] vertexFieldTypes)
        {
            this.data = submesh;
            this.vertexBlockSizeInBytes = vertexBlockSizeInBytes;
            this.vertexBlockFieldPositions = vertexBlockFieldPositions;
            this.vertexBlockFlags = vertexBlockFlags;
            this.vertexFieldTypes = vertexFieldTypes;
        }

        public override void Build()
        {
            this.renderable = true;

            if (this.data == null || this.vertexData == null || this.indexData == null || this.vertexFieldTypes == null || this.vertexBlockFieldPositions == null)
            {
                this.renderable = false;
                return;
            }

            if (this.data.unk16 == 10)
            {
                this.renderable = false;
            }

            bool hasPositions = vertexBlockFlags.HasFlag(FileFormats.M3.Geometry.VertexBlockFlags.hasPosition);
            bool hasTangents = vertexBlockFlags.HasFlag(FileFormats.M3.Geometry.VertexBlockFlags.hasTangent);
            bool hasNormals = vertexBlockFlags.HasFlag(FileFormats.M3.Geometry.VertexBlockFlags.hasNormal);
            bool hasBiTangents = vertexBlockFlags.HasFlag(FileFormats.M3.Geometry.VertexBlockFlags.hasBiTangent);
            bool hasBoneIndices = vertexBlockFlags.HasFlag(FileFormats.M3.Geometry.VertexBlockFlags.hasBoneIndices);
            bool hasBoneWeights = vertexBlockFlags.HasFlag(FileFormats.M3.Geometry.VertexBlockFlags.hasBoneWeights);
            bool hasColors0 = vertexBlockFlags.HasFlag(FileFormats.M3.Geometry.VertexBlockFlags.hasVertexColor0);
            bool hasColors1 = vertexBlockFlags.HasFlag(FileFormats.M3.Geometry.VertexBlockFlags.hasVertexColor1);
            bool hasUV0 = vertexBlockFlags.HasFlag(FileFormats.M3.Geometry.VertexBlockFlags.hasUV0);
            bool hasUV1 = vertexBlockFlags.HasFlag(FileFormats.M3.Geometry.VertexBlockFlags.hasUV1);
            bool hasUnknown = vertexBlockFlags.HasFlag(FileFormats.M3.Geometry.VertexBlockFlags.hasUnknown);

            this.positionCompressed = vertexFieldTypes[0] == FileFormats.M3.Geometry.VertexFieldType.Vector3_16bit;

            GL.GenVertexArrays(1, out VAO);
            VBO = GL.GenBuffer();
            EBO = GL.GenBuffer();

            // bind the Vertex Array Object first, then bind and set vertex buffer(s), and then configure vertex attributes(s).
            GL.BindVertexArray(VAO);

            GL.BindBuffer(BufferTarget.ArrayBuffer, VBO);

            GL.BufferData(BufferTarget.ArrayBuffer, this.vertexData.Length, this.vertexData, BufferUsageHint.StaticDraw);

            GL.BindBuffer(BufferTarget.ElementArrayBuffer, EBO);
            GL.BufferData(BufferTarget.ElementArrayBuffer, this.indexData.Length * 4, this.indexData, BufferUsageHint.StaticDraw);

            int idx = 0;

            if (hasPositions)
            {
                GL.VertexAttribPointer(0, 3, this.positionCompressed ? VertexAttribPointerType.Short : VertexAttribPointerType.Float, false, this.vertexBlockSizeInBytes, this.vertexBlockFieldPositions[idx]);
                GL.EnableVertexAttribArray(0);
            }
            idx++;

            if (hasTangents)
            {
                GL.VertexAttribPointer(1, 2, VertexAttribPointerType.UnsignedByte, false, this.vertexBlockSizeInBytes, this.vertexBlockFieldPositions[idx]);
                GL.EnableVertexAttribArray(1);
            }
            idx++;

            if (hasNormals)
            {
                GL.VertexAttribPointer(2, 2, VertexAttribPointerType.UnsignedByte, false, this.vertexBlockSizeInBytes, this.vertexBlockFieldPositions[idx]);
                GL.EnableVertexAttribArray(2);
            }
            idx++;

            if (hasBiTangents)
            {
                GL.VertexAttribPointer(3, 2, VertexAttribPointerType.UnsignedByte, false, this.vertexBlockSizeInBytes, this.vertexBlockFieldPositions[idx]);
                GL.EnableVertexAttribArray(3);
            }
            idx++;

            if (hasBoneIndices)
            {
                GL.VertexAttribPointer(4, 4, VertexAttribPointerType.UnsignedByte, false, this.vertexBlockSizeInBytes, this.vertexBlockFieldPositions[idx]);
                GL.EnableVertexAttribArray(4);
            }
            idx++;

            if (hasBoneWeights)
            {
                GL.VertexAttribPointer(5, 4, VertexAttribPointerType.UnsignedByte, true, this.vertexBlockSizeInBytes, this.vertexBlockFieldPositions[idx]);
                GL.EnableVertexAttribArray(5);
            }
            idx++;

            if (hasColors0)
            {
                GL.VertexAttribPointer(6, 4, VertexAttribPointerType.UnsignedByte, true, this.vertexBlockSizeInBytes, this.vertexBlockFieldPositions[idx]);
                GL.EnableVertexAttribArray(6);
            }
            idx++;

            if (hasColors1)
            {
                GL.VertexAttribPointer(7, 4, VertexAttribPointerType.UnsignedByte, true, this.vertexBlockSizeInBytes, this.vertexBlockFieldPositions[idx]);
                GL.EnableVertexAttribArray(7);
            }
            idx++;

            if (hasUV0)
            {
                GL.VertexAttribPointer(8, 2, VertexAttribPointerType.HalfFloat, false, this.vertexBlockSizeInBytes, this.vertexBlockFieldPositions[idx]);
                GL.EnableVertexAttribArray(8);
            }
            idx++;

            if (hasUV1)
            {
                GL.VertexAttribPointer(9, 2, VertexAttribPointerType.HalfFloat, false, this.vertexBlockSizeInBytes, this.vertexBlockFieldPositions[idx]);
                GL.EnableVertexAttribArray(9);
            }
            idx++;

            if (hasUnknown)
            {
                GL.VertexAttribPointer(10, 1, VertexAttribPointerType.UnsignedByte, false, this.vertexBlockSizeInBytes, this.vertexBlockFieldPositions[idx]);
                GL.EnableVertexAttribArray(10);
            }
            // note that this is allowed, the call to glVertexAttribPointer registered VBO as the vertex attribute's bound vertex buffer object so afterwards we can safely unbind
            GL.BindBuffer(BufferTarget.ArrayBuffer, 0);

            //buffer = GL.GenBuffer();
            //GL.BindBuffer(BufferTarget.ArrayBuffer, buffer);


            // remember: do NOT unbind the EBO while a VAO is active as the bound element buffer object IS stored in the VAO; keep the EBO bound.
            //glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

            // You can unbind the VAO afterwards so other VAO calls won't accidentally modify this VAO, but this rarely happens. Modifying other
            // VAOs requires a call to glBindVertexArray anyways so we generally don't unbind VAOs (nor VBOs) when it's not directly necessary.
            GL.BindVertexArray(0);

            // Build vertex positions for mouse picking
            this.vertexPositions = new Vector3[this.vertexData.Length / this.vertexBlockSizeInBytes];
            float compressionScale = 1.0f / 1024f;

            int bufferSkip = this.vertexBlockSizeInBytes - (this.positionCompressed ? 6 : 12);
            using (var ms = new MemoryStream(this.vertexData))
            {
                using (BinaryReader br = new BinaryReader(ms))
                {
                    for (int i = 0; i < this.vertexPositions.Length; i++)
                    {
                        this.vertexPositions[i] = this.positionCompressed ? new Vector3(br.ReadInt16(), br.ReadInt16(), br.ReadInt16()) * compressionScale : br.ReadVector3();
                        br.BaseStream.Position += bufferSkip;
                    }
                }
            }

            this.isBuilt = true;
        }

        public override void Draw()
        {
            if (!this.isBuilt || !this.renderable || this.data == null || this.indexData == null) return;

            GL.BindVertexArray(this.VAO);
            GL.DrawElements(BeginMode.Triangles, this.indexData.Length, DrawElementsType.UnsignedInt, 0);
        }

        public override void DrawInstanced()
        {
            if (!this.isBuilt || !this.renderable || this.data == null || this.indexData == null || this.instances == null) return;

            // configure instanced array
            // -------------------------
            //int buffer = GL.GenBuffer();
            GL.BindBuffer(BufferTarget.ArrayBuffer, this.instanceBuffer);
            GL.BufferData(BufferTarget.ArrayBuffer, this.instances.Length * 64, this.instances, BufferUsageHint.StaticDraw);

            GL.BindVertexArray(this.VAO);
            GL.DrawElementsInstanced(PrimitiveType.Triangles, this.indexData.Length, DrawElementsType.UnsignedInt, this.indexData, this.instances.Length);
        }

        public override bool MeshIntersectsRay(Ray ray, Vector3 wPos, Quaternion wRot, Vector3 wScale, ref Vector3[] points)
        {
            if (this.data == null) return false;
            if (this.indexData == null) return false;
            if (this.vertexPositions == null) return false;

            // Transform the ray into the local space of the OBB
            Vector3 rayOrigin = ray.origin - wPos;
            Vector3 rayDirection = ray.direction;
            Quaternion inverseOrientation = Quaternion.Invert(wRot);
            rayOrigin = inverseOrientation * rayOrigin;
            rayDirection = inverseOrientation * rayDirection;
            rayOrigin /= wScale;
            Ray transformedRay = new Ray(rayOrigin, rayDirection);

            int triIdx = 0;
            bool haveIntersection = false;
            for (int i = 0; i < this.indexData.Length; i += 3)
            {
                uint i0 = this.indexData[i];
                uint i1 = this.indexData[i + 1];
                uint i2 = this.indexData[i + 2];

                var v0 = this.vertexPositions[i0];
                var v1 = this.vertexPositions[i1];
                var v2 = this.vertexPositions[i2];

                if (this.TriangleIntersectsRay(transformedRay, v0, v1, v2, out points[triIdx]))
                {
                    // Transform the point to world space
                    points[triIdx] *= wScale;
                    points[triIdx] = wRot * points[triIdx];
                    points[triIdx] += wPos;

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
            this.renderable = false;
            this.indexData = null;
            this.vertexData = null;
            this.vertexPositions = null;
            this.instances = null;

            GL.DeleteVertexArray(this.VAO);
            GL.DeleteBuffer(this.VBO);
            GL.DeleteBuffer(this.EBO);
        }
    }
}
