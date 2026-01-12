using ProjectWS.Engine.Material;
using ProjectWS.Engine.Mesh;
using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace ProjectWS.Engine.World
{
    public class M3Geometry
    {
        FileFormats.M3.Geometry? data;
        public M3Mesh[]? meshes;
        public bool isBuilt;

        public M3Geometry(FileFormats.M3.Geometry data)
        {
            this.data = data;
            if (data.submeshes != null)
                meshes = new M3Mesh[data.submeshes.Length];
        }

        internal void Build(int m3ModelID)
        {
            if (this.data == null || this.meshes == null || this.data.submeshes == null ||
                this.data.vertexData == null || this.data.indexData == null ||
                this.data.vertexBlockFieldPositions == null) return;

#pragma warning disable CS8604 // Possible null reference argument.
            for (int i = 0; i < meshes.Length; i++)
            {
                var submesh = this.data.submeshes[i];

                this.meshes[i] = new M3Mesh(submesh, this.data.vertexBlockSizeInBytes, this.data.vertexBlockFieldPositions, this.data.vertexBlockFlags, this.data.vertexFieldTypes);

                this.meshes[i].vertexData = new byte[submesh.vertexCount * this.data.vertexBlockSizeInBytes];
                Array.Copy(data.vertexData, submesh.startVertex * this.data.vertexBlockSizeInBytes, this.meshes[i].vertexData, 0, submesh.vertexCount * this.data.vertexBlockSizeInBytes);

                this.meshes[i].indexData = new uint[submesh.indexCount];
                Array.Copy(data.indexData, submesh.startIndex, this.meshes[i].indexData, 0, submesh.indexCount);

                this.meshes[i].Build();

                if (submesh.meshGroupID == m3ModelID || submesh.meshGroupID == -1)
                {
                    this.meshes[i].renderable = true;
                }
                else
                {
                    this.meshes[i].renderable = false;
                }
            }
#pragma warning restore CS8604 // Possible null reference argument.

            isBuilt = true;
        }
    }
}
