using MathUtils;
using System.Collections.Concurrent;

namespace ProjectWS.Engine.World
{
    public class Chunk
    {
        #region Variables

        public World? world;
        public string? areaFilePath;
        public string? areaLowFilePath;
        public Vector2 coords;
        Vector2 lowCoords;
        public Vector3 worldCoords;
        public Matrix4 worldMatrix;
        public volatile bool isVisible;
        int lod = -1;
        public bool lod0Available;
        public bool lod0Loading;
        public bool lod1Available;
        public bool lod1Loading;
        int[] lodQuadrants;
        public float minHeight;
        public float maxHeight;

        public AABB? AABB;
        public FileFormats.Area.File? area;
        public FileFormats.Area.File? areaLow;
        public List<SubChunk>? subChunks;

        const float LABEL_DISTANCE = 300f;

        public readonly Vector3[] corners = new Vector3[]
        {
            new Vector3(-1, 0, -1),
            new Vector3(1, 0, -1),
            new Vector3(-1, 0, 1),
            new Vector3(1, 0, 1)
        };

        // Tasks
        public ConcurrentQueue<TaskManager.TerrainTask>? terrainTasks;
        public ConcurrentQueue<TaskManager.TerrainTask>? buildTasks;
        internal bool modified;

        #endregion

        /// <summary>
        /// Load chunk from project
        /// </summary>
        /// <param name="coords"></param>
        /// <param name="file"></param>
        /// <param name="data"></param>
        /// <param name="world"></param>
        public Chunk(Vector2 coords, string file, World world)
        {
            this.terrainTasks = new ConcurrentQueue<TaskManager.TerrainTask>();
            this.buildTasks = new ConcurrentQueue<TaskManager.TerrainTask>();

            this.world = world;
            this.areaFilePath = file;
            this.coords = coords;
            this.worldCoords = Utilities.ChunkToWorldCoords(coords);
            this.worldMatrix = new Matrix4().TRS(this.worldCoords, Quaternion.Identity, new Vector3(1, 1, 1));
            this.lowCoords = Utilities.CalculateLowCoords(coords);
            this.lodQuadrants = Utilities.CalculateLoDQuadrants(coords, this.lowCoords);
            this.AABB = new AABB(this.worldCoords, new Vector3(512f, 10000f, 512f));
            this.subChunks = new List<SubChunk>();
            this.minHeight = float.MaxValue;
            this.maxHeight = float.MinValue;
        }

        /// <summary>
        /// Create new flat chunk
        /// </summary>
        /// <param name="coords"></param>
        /// <param name="data"></param>
        /// <param name="world"></param>
        public Chunk(Vector2 coords, World world)
        {
            this.terrainTasks = new ConcurrentQueue<TaskManager.TerrainTask>();
            this.buildTasks = new ConcurrentQueue<TaskManager.TerrainTask>();

            this.world = world;
            this.coords = coords;
            this.worldCoords = Utilities.ChunkToWorldCoords(coords);
            this.worldMatrix = new Matrix4().TRS(this.worldCoords, Quaternion.Identity, new Vector3(1, 1, 1));
            this.lowCoords = Utilities.CalculateLowCoords(coords);
            this.lodQuadrants = Utilities.CalculateLoDQuadrants(coords, this.lowCoords);
            this.AABB = new AABB(this.worldCoords, new Vector3(512f, 10000f, 512f));
            this.subChunks = new List<SubChunk>();
            this.minHeight = float.MaxValue;
            this.maxHeight = float.MinValue;
        }

        public void EnqueueTerrainTask(TaskManager.TerrainTask task) => this.terrainTasks?.Enqueue(task);

        public void EnqueueBuildTask(TaskManager.TerrainTask task) => this.buildTasks?.Enqueue(task);

        public int GetLoDQuadrant(int index) => this.lodQuadrants[index];

        public void RenderTerrain(Shader terrainShader)
        {
            if (!this.isVisible)
                return;

            if (this.lod != -1)
            {
                if (this.lod == 0)
                {
                    bool rendered = false;
                    if (this.lod0Available)
                    {
                        RenderLod0Terrain(terrainShader);
                        rendered = true;
                    }

                    if (this.lod1Available && !rendered)
                    {
                        RenderLod1Terrain();
                        rendered = true;
                    }
                }
                else if (this.lod == 1)
                {
                    if (this.lod1Available)
                    {
                        RenderLod1Terrain();
                    }
                }
            }
        }

        public void RenderWater(Shader terrainShader)
        {
            if (!this.isVisible)
                return;

            if (this.lod != -1)
            {
                if (this.lod == 0)
                {
                    bool rendered = false;
                    if (this.lod0Available)
                    {
                        RenderLod0Water(terrainShader);
                        rendered = true;
                    }

                    if (this.lod1Available && !rendered)
                    {
                        RenderLod1Water();
                        rendered = true;
                    }
                }
                else if (this.lod == 1)
                {
                    if (this.lod1Available)
                    {
                        RenderLod1Water();
                    }
                }
            }
        }

        void RenderLod0Terrain(Shader shader)
        {
            if (this.area == null) return;

            if (this.area.subAreas == null) return;

            for (int i = 0; i < this.subChunks.Count; i++)
            {
                if (!this.subChunks[i].isVisible)
                    continue;

                if (this.subChunks[i].terrainMesh != null)
                {
                    Rendering.WorldRenderer.drawCalls++;
                    shader.SetMat4("model", ref this.subChunks[i].matrix);

                    this.subChunks[i].terrainMaterial.SetToShader(shader);
                    this.subChunks[i].terrainMesh.Draw();
                }
            }
        }

        void RenderLod0Water(Shader shader)
        {
            if (this.area == null) return;

            for (int i = 0; i < this.subChunks?.Count; i++)
            {
                if (!this.subChunks[i].isVisible)
                    continue;

                if (this.area.subAreas[i].hasWater)
                {
                    for (int j = 0; j < this.subChunks[i].waterMeshes.Length; j++)
                    {
                        this.subChunks[i].waterMaterials[j].SetToShader(shader);
                        this.subChunks[i].waterMeshes[j].Draw();
                    }
                }
            }
        }

        public void RenderDebug()
        {
            if (this.area == null) return;
            if (this.area.subAreas == null) return;

            for (int i = 0; i < this.subChunks.Count; i++)
            {
                if (this.subChunks[i] == null)
                    continue;

                if (!this.subChunks[i].isVisible)
                    continue;

                if (this.subChunks[i].terrainMesh != null)
                {
                    var pos = this.subChunks[i].centerPosition;

                    if (this.subChunks[i].distanceToCam < LABEL_DISTANCE)
                    {
                        /*
                        if (this.world.controller.subchunkIndex == i)
                        {
                            float fade = MathF.Max(MathF.Min(1.0f - (this.area.subChunks[i].distanceToCam / LABEL_DISTANCE), 1.0f), 0.0f);
                            if (this.areaFileEntry != null)
                                Debug.DrawLabel($"{this.areaFileEntry.name} | {i}", pos, new Vector4(1.0f, 0.5f, 1.0f, fade), true);
                            else if (this.areaFilePath != null)
                                Debug.DrawLabel($"{this.areaFilePath} | {i}", pos, new Vector4(1.0f, 0.5f, 1.0f, fade), true);
                        }
                        */
                        /*
                        for (int j = 0; j < 4; j++)
                        {
                            var corner = this.area.subChunks[i].skyCorners[j];

                            if (corner != null)
                            {
                                //this.currentWorldSkyID = corner.worldSkyIDs[j];
                                //var worldSkyRecord = this.database.worldSky.Get(corner.worldSkyIDs[j]);
                                if (this.world.controller.subchunkIndex == i)
                                {
                                    //Debug.DrawLabel(j.ToString(), pos + (directions[j] * 16.0f), new Vector4(1.0f, 1.0f, 0.0f, 1.0f), true);
                                    Debug.DrawLabel(corner.worldSkyIDs[3].ToString(), pos + (corners[j] * 16.0f), new Vector4(1.0f, 1.0f, 0.0f, 1.0f), true);
                                }
                                else
                                {
                                    Debug.DrawLabel(corner.worldSkyIDs[3].ToString(), pos + (corners[j] * 16.0f), Vector4.One, true);
                                }
                            }
                        }
                        */
                    }
                }
            }
        }

        void RenderLod1Terrain()
        {
            if (this.areaLow != null)
            {

            }
        }

        void RenderLod1Water()
        {
            if (this.areaLow != null)
            {

            }
        }

        public void SetLod(int lod, Vector2 center)
        {
            if (this.lod != lod)
            {
                this.lod = lod;
            }

            switch (this.lod)
            {
                case 0:
                    LoadLod1();
                    LoadLod0();
                    break;
                case 1:
                    LoadLod1();
                    break;
            }

            if (this.lod == -1)
            {
                // TOOD: Later on, check how many chunks are loaded in memory, and if this is an older unloaded one then actually dispose of it to free memory
            }
        }

        void LoadLod0()
        {
            if (!this.lod0Available && !this.lod0Loading)
            {
                this.lod0Loading = true;

                this.area = new FileFormats.Area.File(this.areaFilePath);
                this.terrainTasks.Enqueue(new TaskManager.TerrainTask(this, 0, TaskManager.Task.JobType.Read));
            }
        }

        void LoadLod1()
        {
            /*
            if (!this.lod1Available && !this.lod1Loading)
            {
                this.lod1Loading = true;

                this.areaLow = new Area(this, 1);
                this.terrainTasks.Enqueue(new TaskManager.TerrainTask(this, 1, TaskManager.Task.JobType.Read));
            }
            */
        }

        public void CalculateCulling(Camera camera/*, Light sunLight*/)
        {
            //Vector3 sunVector = sunLight.lightObj.transform.forward;      // Main thread

            // Chunk //

            // Frustum Culling //
            if (camera == null) return;

            if (camera is PerspectiveCamera)
            {
                var pCamera = camera as PerspectiveCamera;
                if (pCamera == null) return;

                this.isVisible = pCamera.frustum.VolumeVsFrustum(this.AABB.center + (this.AABB.extents / 2), this.AABB.extents.X, this.AABB.extents.Y, this.AABB.extents.Z);

                // SubChunks //
                if (this.isVisible)
                {
                    if (this.lod0Available)
                    {
                        for (int i = 0; i < this.subChunks?.Count; i++)
                        {
                            // Reset occlusion //
                            this.subChunks[i].isOccluded = false;

                            // Distance Culling //
                            this.subChunks[i].distanceToCam = Math.Abs(Vector3.Distance(this.subChunks[i].centerPosition, camera.transform.GetPosition()));
                            this.subChunks[i].isCulled = this.subChunks[i].distanceToCam > 1024f;  // 1024 being the distance between 2 chunks

                            // Frustum Culling //
                            if (!this.subChunks[i].isCulled)
                            {
                                var aabb = this.subChunks[i].AABB;
                                this.subChunks[i].isCulled = !pCamera.frustum.VolumeVsFrustum(aabb.center + (aabb.extents / 2), aabb.extents.X, aabb.extents.Y, aabb.extents.Z);
                            }
                        }
                    }
                }
            }
        }

        internal void Build(int lod, int quadrant)
        {
            if (lod == 0)
            {
                int from = quadrant * (256 / 4);
                int to = from + (256 / 4);

                if (this.area == null)
                {
                    return;
                }

                if (this.area.subAreas == null)
                {
                    return;
                }

                if (this.subChunks == null)
                {
                    return;
                }

                int total = this.area.subAreas.Count;
                for (int i = from; i < to; i++)
                {
                    if (i < total)
                    {
                        // Terrain //
                        this.subChunks[i]?.terrainMesh?.Build();
                        this.subChunks[i]?.terrainMaterial?.Build();

                        // Water //
                        if (this.subChunks[i].waterMeshes != null)
                        {
                            for (int j = 0; j < this.subChunks[i]?.waterMeshes?.Length; j++)
                            {
                                this.subChunks[i]?.waterMeshes?[j]?.Build();
                            }
                        }
                    }
                }

                this.lod0Available = true;
            }
            else if (lod == 1)
            {
                this.lod1Available = true;
            }
        }

        internal void ReadArea(FileFormats.Area.File area)
        {
            if (this.areaFilePath != null)
            {
                using(var fs = File.OpenRead(this.areaFilePath))
                {
                    area.Read(fs);
                }
            }

            // Create subchunks
            for (int i = 0; i < area.subAreas?.Count; i++)
            {
                this.subChunks.Add(new SubChunk(this, area.subAreas[i]));
            }
        }

        internal void ReadAreaLow(FileFormats.Area.File areaLow)
        {
            if (this.areaLowFilePath != null)
            {
                using (var fs = File.OpenRead(this.areaLowFilePath))
                {
                    areaLow.Read(fs);
                }
            }

            for (int i = 0; i < areaLow.subAreas?.Count; i++)
            {
                var sc = new SubChunk(this, areaLow.subAreas[i]);
                //sc.mesh = new TerrainMesh(areaLow.subAreas[i].lodHeightMap, areaLow.subAreas[i]);
                //this.subChunksLow.Add(sc);
            }
        }

        internal void Unload()
        {
            this.terrainTasks?.Clear();
            this.buildTasks?.Clear();

            this.areaFilePath = null;
            this.areaLowFilePath = null;

            this.lod0Available = false;
            this.lod0Loading = false;
            this.lod1Available = false;
            this.lod1Loading = false;

            this.minHeight = float.MaxValue;
            this.maxHeight = float.MinValue;

            this.AABB = null;
            this.area = null;
            this.areaLow = null;

            for (int i = 0; i < this.subChunks?.Count; i++)
            {
                this.subChunks[i].Unload();
            }

            this.subChunks?.Clear();
            this.subChunks = null;
        }
    }
}
