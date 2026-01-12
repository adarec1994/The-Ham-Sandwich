using MathUtils;
using ProjectWS.Engine.Material;
using ProjectWS.Engine.Mesh;

namespace ProjectWS.Engine.World
{
    public class SubChunk
    {
        public volatile bool isVisible;
        public bool isOccluded;
        public bool isCulled;
        public Chunk? chunk;

        public FileFormats.Area.SubArea? subArea;

        public TerrainMesh? terrainMesh;
        public WaterMesh[]? waterMeshes;
        public TerrainMaterial? terrainMaterial;
        public WaterMaterial[]? waterMaterials;

        public AABB? AABB;
        public AABB? cullingAABB;
        public Vector3 centerPosition;
        public int X;
        public int Y;
        public Matrix4 matrix;
        public Vector2 subCoords;
        public float distanceToCam;

        public SubChunk(Chunk chunk, FileFormats.Area.SubArea subArea)
        {
            this.chunk = chunk;
            this.subArea = subArea;

            this.terrainMesh = new TerrainMesh(this.subArea.heightMap, this);
            this.terrainMaterial = new Material.TerrainMaterial(this.subArea);
            if (this.subArea.hasWater)
            {
                this.waterMeshes = new WaterMesh[this.subArea.waters.Length];
                for (int i = 0; i < this.subArea.waters.Length; i++)
                {
                    this.waterMeshes[i] = new WaterMesh(this.subArea.waters[i]);
                }

                this.waterMaterials = new WaterMaterial[this.subArea.waters.Length];
                for (int i = 0; i < this.subArea.waters.Length; i++)
                {
                    this.waterMaterials[i] = new WaterMaterial(this.subArea.waters[i]);
                }
            }

            // Calc minmax
            if (this.terrainMesh.minHeight < this.chunk.minHeight)
                this.chunk.minHeight = this.terrainMesh.minHeight;
            if (this.terrainMesh.maxHeight > this.chunk.maxHeight)
                this.chunk.maxHeight = this.terrainMesh.maxHeight;

            // Calc Model Matrix
            int chunkX = subArea.index % 16;
            int chunkY = subArea.index / 16;
            this.X = chunkX;
            this.Y = chunkY;
            this.subCoords = (this.chunk.coords * 16) + new Vector2(chunkX, chunkY);
            Vector3 subchunkRelativePosition = new Vector3(chunkX * 32f, 0, chunkY * 32f);
            this.matrix = Matrix4.CreateTranslation(subchunkRelativePosition);
            this.matrix *= chunk.worldMatrix;

            // Calc AABB
            float hMin = this.terrainMesh.minHeight;
            float hMax = this.terrainMesh.maxHeight;
            this.centerPosition = chunk.worldCoords + subchunkRelativePosition + new Vector3(16f, ((hMax - hMin) / 2f) + hMin, 16f);
            this.AABB = new AABB(this.centerPosition, new Vector3(32f, hMax - hMin, 32f));            // Exact bounds
            this.cullingAABB = new AABB(this.centerPosition, new Vector3(64f, (hMax - hMin) * 2, 64f));        // Increased bounds to account for thread delay
        }

        internal void Unload()
        {
            this.terrainMesh?.Unload();
            for (int i = 0; i < this.waterMeshes?.Length; i++)
            {
                this.waterMeshes[i].Unload();
            }
            this.waterMeshes = null;
            this.terrainMaterial?.Unload();
            for (int i = 0; i < this.waterMaterials?.Length; i++)
            {
                this.waterMaterials[i].Unload();
            }
            this.waterMaterials = null;
            this.AABB = null;
            this.cullingAABB = null;
        }
    }
}
