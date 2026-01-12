using MathUtils;
using OpenTK.Graphics.OpenGL4;
using ProjectWS.Engine.Rendering;
using System.Collections.Concurrent;

namespace ProjectWS.Engine.World
{
    public class World
    {
        #region Variables

        public const int WORLD_SIZE = 128;
        public const float AREA_SIZE = 512.0f;
        public const float SUBCHUNK_SIZE = 32.0f;
        public const int DRAWDIST0 = 2;
        public const int DRAWDIST1 = 2;
        public const int DRAWDIST2 = 2;

        // Refs
        public Engine? engine;
        public Rendering.WorldRenderer? renderer;
        public Environment? environment;

        // Map
        uint loadedWorldID;
        public Dictionary<Vector2i, Chunk>? chunks;
        public Dictionary<Vector2i, Chunk>? activeChunks;

        // Prop
        public Dictionary<string, Prop>? props;

        // Controller
        public Controller controller;

        // Culling
        Thread? cullingThread;
        bool cullingThreadRunning;
        System.Diagnostics.Stopwatch? cullingStopwatch;
        float cullingFrametime;
        volatile bool cullTaskProcess;
        volatile bool cullTaskUpdate;

        #endregion

        public World(Engine engine)
        {
            this.chunks = new Dictionary<Vector2i, Chunk>();
            this.activeChunks = new Dictionary<Vector2i, Chunk>();
            this.props = new Dictionary<string, Prop>();
            this.controller = new Controller(this);
            this.controller.onChunkPositionChange = OnChunkPositionChange;
            this.controller.onSubchunkPositionChange = OnSubchunkPositionChange;
            this.controller.onWorldPositionChange = OnWorldPositionChange;
            this.cullingStopwatch = new System.Diagnostics.Stopwatch();
            this.environment = new Environment(this);

            this.engine = engine;

            if (FindRenderer())
            {
                if (this.renderer != null)
                {
                    Debug.Log("Set World");
                    this.renderer.SetWorld(this);
                }
            }
        }

        public void CreateNew(string worldName)
        {
            /*
            //string installLocation = @"G:\Reverse Engineering\GameData\Wildstar 1.7.8.16042\";
            //var worldTblPath = installLocation + @"Data\DB\World.tbl";
            //var worldTbl = Tbl<Database.Definitions.World>.Open(worldTblPath);

            var worldTbl = this.gameData.database.world;

            // WorldLocation2 - Find available ID
            uint newWorldID = (uint)worldTbl.lookup.Count;//(worldTbl.header.maxID + 1);
            
            // Create DB entry
            worldTbl.Add(new Database.Definitions.World()
            {
                ID = newWorldID,
                assetPath = $"Map\\{worldName}",
                flags = 0,
                type = 0,
                screenPath = "",
                screenModelPath = "",
                chunkBounds00 = 0,
                chunkBounds01 = 0,
                chunkBounds02 = 0,
                chunkBounds03 = 0,
                plugAverageHeight = 0,
                localizedTextIdName = 0,
                minItemLevel = 0,
                maxItemLevel = 0,
                primeLevelOffset = 0,
                primeLevelMax = 0,
                veteranTierScalingType = 0,
                heroismMenaceLevel = 0,
                rewardRotationContentId = 0
            }, newWorldID);
            var mapDir = $"{this.gameData.gamePath}\\Data\\Map\\{worldName}";
            if (!Directory.Exists(mapDir))
                Directory.CreateDirectory(mapDir);

            Teleport(0, 0, 0, newWorldID, 1);

            CreateChunk(new Vector2i(64, 64), worldName);

            //worldTbl.Write();
            */
        }

        public void CreateChunk(Vector2i coords, string worldName)
        {
            /*
            var mapDir = $"{this.gameData.gamePath}\\Data\\Map\\{worldName}";
            var chunk = new Chunk(coords, this.gameData, this);
            string x = coords.X.ToString("X").ToLower();
            string y = coords.Y.ToString("X").ToLower();
            chunk.areaFilePath = $"{mapDir}\\{worldName}.{y}{x}.area";
            var area = new FileFormats.Area.File(chunk.areaFilePath);
            area.Create();
            area.AddProp("Art\\Dev\\gray_sphere.m3", new Vector3(256, -995, 254), Quaternion.Identity, 5.0f);
            area.Write();
            */
        }

        public void TeleportToWorldLocation(uint ID, int projectID)
        {
            /*
            var record = this.database.worldLocation.Get(ID);

            if (record == null)
            {
                Debug.Log($"WorldLocation database doesn't contain record {ID}");
                return;
            }

            Teleport(record.position0, record.position1, record.position2, record.worldId, projectID);
            */
        }

        public void LoadMap(uint ID, string projectPath, string assetPath, string mapName, List<Vector2i> availableChunks, Vector3 position)
        {
            Teleport(position.X, position.Y, position.Z);
            LoadMap(ID, projectPath, assetPath, mapName, availableChunks);
        }

        public void LoadMap(uint ID, string projectPath, string assetPath, string mapName, List<Vector2i> availableChunks)
        {
            if (this.loadedWorldID == ID)
            {
                // World already loaded, teleport instead
                return;
            }

            if (this.loadedWorldID != 0)
            {
                // A map is already loaded or loading, dispose of it
                UnloadMap();
            }

            this.loadedWorldID = ID;

            CreateChunks(availableChunks, projectPath, assetPath, mapName);

            this.controller.spawn = true;
            this.controller.chunkPosition = Utilities.WorldToChunkCoords(this.controller.worldPosition);
            Debug.Log($"Start: chunk {this.controller.chunkPosition} | world {this.controller.worldPosition}");
        }

        public void UnloadMap()
        {
            if (this.engine?.taskManager != null)
            {
                this.engine.taskManager.terrainThread.tasks.Clear();
                this.engine.taskManager.modelThread.tasks.Clear();
                this.engine.taskManager.textureThread.tasks.Clear();
                this.engine.taskManager.otherThread.tasks.Clear();
                this.engine.taskManager.buildTasks.Clear();
            }

            if (this.chunks != null)
            {
                for (int x = 0; x < WORLD_SIZE; x++)
                {
                    for (int y = 0; y < WORLD_SIZE; y++)
                    {
                        if (this.chunks.TryGetValue(new Vector2i(x, y), out var chunk))
                            chunk.Unload();
                    }
                }

                this.chunks.Clear();
            }

            if (this.props != null)
            {
                foreach (var item in this.props)
                {
                    item.Value.Unload();
                }

                //this.props.Clear();
            }

        }

        public void Teleport(float x, float y, float z)
        {
            Debug.Log($"Teleport {x} {y} {z}");
            this.controller.worldPosition = new Vector3(x, y, z);

            if (this.renderer == null)
            {
                if (FindRenderer())
                {
                    if (this.renderer != null)
                    {
                        Debug.Log("Set World");
                        this.renderer.SetWorld(this);
                    }
                }
            }

            var mainCam = this.renderer.viewports[0].mainCamera;
            var camController = mainCam.components[0] as Components.CameraController;
            mainCam.transform.SetPosition(this.controller.worldPosition);
            camController.Teleport(this.controller.worldPosition.X, this.controller.worldPosition.Y, this.controller.worldPosition.Z);
        }

        internal void LoadProp(FileFormats.M3.File data, FileFormats.Area.AreaProp areaprop)
        {
            if (this.props == null) return;

            if (this.props.ContainsKey(data.filePath))
            {
                this.props[data.filePath].AddInstance(areaprop);
            }
            else
            {
                var prop = new Prop(data, areaprop, this.engine);
                this.props.Add(data.filePath, prop);
            }
        }

        public void Update(float deltaTime)
        {
            // Visibility / Loading / Culling
            if (this.controller != null && this.renderer != null && this.renderer.viewports != null)
            {
                for (int v = 0; v < this.renderer.viewports.Count; v++)
                {
                    if (this.renderer.viewports[v] != null)
                        this.controller.Update(this.renderer.viewports[v].mainCamera);
                }
            }

            CullingMT();
            TasksUpdate();
        }

        public void RenderTerrain(Shader shader)
        {
            if (this.activeChunks == null) return;

            GL.Disable(EnableCap.Blend);
            GL.Enable(EnableCap.DepthTest);

            foreach (KeyValuePair<Vector2i, Chunk> chunk in this.activeChunks)
            {
                chunk.Value.RenderTerrain(shader);
                chunk.Value.RenderDebug();
            }
        }

        public void RenderWater(Shader shader)
        {
            if (this.activeChunks == null) return;

            GL.Disable(EnableCap.Blend);
            GL.Enable(EnableCap.DepthTest);

            foreach (KeyValuePair<Vector2i, Chunk> chunk in this.activeChunks)
            {
                chunk.Value.RenderWater(shader);
            }
        }

        public void RenderProps(Shader shader)
        {
            if (this.activeChunks == null) return;
            if (this.props == null) return;

            GL.Disable(EnableCap.Blend);
            GL.Enable(EnableCap.DepthTest);

            foreach (var item in this.props)
            {
                var prop = item.Value;

                for (int k = 0; k < prop.renderableInstances?.Count; k++)
                {
                    if (prop.hasMeshes)
                    {
                        Matrix4 model = prop.renderableInstances[k];
                        prop.Render(shader, model);
                    }
                    else
                    {
                        // TODO : check if it's actually a light m3 or just some other thing like a camera or whatever
                        Debug.DrawIcon3D(IconRenderer.Icon3D.Type.Light, prop.renderableInstances[k].ExtractPosition(), Vector4.One);
                    }
                }
            }
        }

        void TasksUpdate()
        {
            if (this.engine == null) return;
            if (this.engine.taskManager == null) return;
            if (this.activeChunks == null) return;

            foreach (var item in this.activeChunks)
            {
                Chunk chunk = item.Value;
                Vector2 coord = item.Key;

                // Terrain Tasks //
                if (chunk.terrainTasks?.Count > 0 && this.engine.taskManager.terrainThread.tasks.Count < 4)
                {
                    if (chunk.terrainTasks.TryDequeue(out TaskManager.TerrainTask? task))
                    {
                        this.engine.taskManager.terrainThread.tasks.Enqueue(task);
                    }
                }

                // Build Object Tasks //
                if (chunk.buildTasks?.Count > 0 && this.engine.taskManager.buildTasks.Count < 4)
                {
                    if (chunk.buildTasks.TryDequeue(out TaskManager.TerrainTask? task))
                        this.engine.taskManager.buildTasks.Enqueue(task);
                }

                /*
                if (chunk.refreshObjectTasks.Count > 0)
                {
                    int count = chunk.refreshObjectTasks.Count;
                    for (int i = 0; i < count; i++)
                    {
                        if (chunk.refreshObjectTasks.TryDequeue(out TaskHandler.Task task))
                        {
                            chunk.lod1.RefreshMesh();
                        }
                    }
                }
                */
            }
        }

        bool FindRenderer()
        {
            if (this.renderer != null) return true;

            // Find first available world renderer and set world
            // Should only be 1 world renderer
            for (int i = 0; i < this.engine?.renderers?.Count; i++)
            {
                if (engine.renderers[i] is Rendering.WorldRenderer)
                {
                    this.renderer = engine.renderers[i] as Rendering.WorldRenderer;
                    return true;
                }
            }

            return false;
        }

        void OnChunkPositionChange(Vector2i cPos)
        {

        }

        void OnSubchunkPositionChange(int index)
        {
            if (this.environment != null)
            {
                this.environment.SubchunkChange();
            }
        }

        void OnWorldPositionChange(Vector3 pos)
        {

        }

        /// <summary>
        /// Main Thread Culling
        /// </summary>
        void CullingMT()
        {
            if (this.loadedWorldID != 0)
            {
                // Boot CullingThread
                if (!this.cullingThreadRunning)
                {
                    this.cullingThreadRunning = true;

                    // Boot the thread //
                    this.cullingThread = new Thread(() => CullingST())
                    {
                        Name = "WRenderer.CullingThread",
                        IsBackground = true,
                        Priority = System.Threading.ThreadPriority.AboveNormal
                    };
                    this.cullingThread.Start();

                    this.cullTaskProcess = true;
                }
            }

            // Update Culling and cycle cullingUpdateQueue->cullingProcessQueue
            if (this.cullTaskUpdate)
            {
                this.cullTaskUpdate = false;

                // Update prop culling
                foreach (var item in this.props)
                {
                    var prop = item.Value;

                    prop.renderableInstances.Clear();
                    prop.renderableUUIDs.Clear();
                    for (int k = 0; k < prop.uniqueInstanceIDs.Count; k++)
                    {
                        uint uuid = prop.uniqueInstanceIDs[k];
                        prop.instances[uuid].visible = false;
                        if (prop.cullingResults[prop.instances[uuid].uuid])
                        {
                            prop.renderableUUIDs.Add(prop.instances[uuid].uuid);
                            prop.renderableInstances.Add(prop.instances[uuid].transform);
                            prop.instances[uuid].visible = true;
                        }

                        prop.culled = prop.renderableInstances.Count == 0;
                    }
                }

                this.cullTaskProcess = true;
            }
        }

        /// <summary>
        /// Secondary Thread Culling
        /// </summary>
        public void CullingST()
        {
            while (this.cullingThreadRunning)
            {
                if (this.cullTaskProcess)
                {
                    this.cullTaskProcess = false;

                    this.cullingStopwatch.Start();

                    bool processed = false;
                    try
                    {
                        // Chunks //
                        ChunksCulling();

                        // Props //
                        PropCulling();
                    }
                    catch (System.Exception e)
                    {
                        Debug.LogException(e);
                    }

                    processed = true;
                    this.cullTaskUpdate = true;

                    this.cullingStopwatch.Stop();
                    if (processed)
                        this.cullingFrametime += ((this.cullingStopwatch.ElapsedMilliseconds / 1000f) - this.cullingFrametime) * 0.03f;
                    this.cullingStopwatch.Restart();
                }
            }
        }

        public void ChunksCulling()
        {
            // Chunks - Frustum and Distance //

            if (this.renderer == null) return;
            if (this.renderer.viewports == null) return;

            foreach (var item in this.activeChunks)
            {
                item.Value.CalculateCulling(this.renderer.viewports[0].mainCamera/*, this.sunLight*/);
            }

            // Chunks - Occlusion //
            //ChunksOcclusionCulling();

            // Set the visibility bools for subchunks //
            foreach (var item in this.activeChunks)
            {
                if (item.Value.isVisible)
                {
                    if (item.Value.lod0Available)
                    {
                        for (int i = 0; i < item.Value.subChunks.Count; i++)
                        {
                            item.Value.subChunks[i].isVisible = !item.Value.subChunks[i].isCulled && !item.Value.subChunks[i].isOccluded;
                        }
                    }
                }
            }
        }

        /*
        public void ChunksOcclusionCulling()
        {
            // TODO : This is very slow, lags the culling thread far behind rendering
            this.distanceSortedSubchunksBuffer.Clear();
            foreach (var item in this.activeChunks)
            {
                if (item.Value.isVisible)
                {
                    if (item.Value.lod0Available)
                    {
                        for (int i = 0; i < item.Value.area.subChunks.Count; i++)
                        {
                            if (!item.Value.area.subChunks[i].isCulled)
                            {
                                this.distanceSortedSubchunksBuffer.Add(item.Value.area.subChunks[i]);
                            }
                        }
                    }
                }
            }
            this.distanceSortedSubchunksBuffer.Sort(this.chunkComparer);
            for (int i = 0; i < this.distanceSortedSubchunksBuffer.Count; i++)
            {
                this.distanceSortedSubchunksBuffer[i].rectBehindCamera = this.distanceSortedSubChunks[i].AABB.GetScreenSpaceRect(Matrix4x4.identity, out this.distanceSortedSubChunks[i].screenRect, this.activeCamera.viewMatrix, this.activeCamera.projectionMatrix);
            }

            for (int i = 0; i < this.distanceSortedSubchunksBuffer.Count; i++)
            {
                if (!this.distanceSortedSubchunksBuffer[i].isCulled)
                {
                    for (int j = i + 1; j < this.distanceSortedSubchunksBuffer.Count; j++)
                    {
                        if (!this.distanceSortedSubchunksBuffer[j].isCulled)
                        {
                            if (!this.distanceSortedSubchunksBuffer[i].rectBehindCamera)
                                continue;

                            //if (this.distanceSortedSubChunks[i].screenRect.Overlaps(this.distanceSortedSubChunks[j].screenRect))
                            //    this.distanceSortedSubChunks[j].isOccluded = true;

                            float xminL = this.distanceSortedSubchunksBuffer[i].screenRect.min.x;
                            float xmaxL = this.distanceSortedSubchunksBuffer[i].screenRect.max.x;
                            float xminS = this.distanceSortedSubchunksBuffer[j].screenRect.min.x;
                            float xmaxS = this.distanceSortedSubchunksBuffer[j].screenRect.max.x;

                            if (xminL < xminS && xmaxL > xmaxS)
                            {
                                // Wider than further
                                float yMinL = this.distanceSortedSubchunksBuffer[i].screenRect.min.y;
                                float yMaxS = this.distanceSortedSubchunksBuffer[j].screenRect.max.y;

                                if (yMinL > yMaxS)
                                {
                                    this.distanceSortedSubchunksBuffer[j].isOccluded = true;
                                }
                            }
                        }
                    }
                }
            }
        }
        */

        public void PropCulling()
        {
            foreach (var item in this.props)
            {
                var prop = item.Value;

                for (int j = 0; j < prop.uniqueInstanceIDs.Count; j++)
                {
                    prop.cullingResults[prop.uniqueInstanceIDs[j]] = false;
                }
            }

            foreach (var item in this.activeChunks)
            {
                if (item.Value.isVisible)
                {
                    if (item.Value.lod0Available)
                    {
                        for (int i = 0; i < item.Value.subChunks.Count; i++)
                        {
                            if (item.Value.subChunks[i].isVisible)
                            {
                                if (item.Value.area.subAreas[i].propUniqueIDs == null) continue;
                                for (int j = 0; j < item.Value.area.subAreas[i].propUniqueIDs.Count; j++)
                                {
                                    var uuid = item.Value.area.subAreas[i].propUniqueIDs[j];
                                    var areaprop = item.Value.area.propLookup[uuid];
                                    var size = areaprop.placement.sizef;
                                    if (areaprop.path == null) continue;

                                    if (item.Value.subChunks[i].distanceToCam / (size * areaprop.scale) < 200f)
                                    {
                                        if (areaprop.loadRequested)
                                        {
                                            if (this.props.TryGetValue(areaprop.path, out Prop? prop))
                                            {
                                                prop.cullingResults[uuid] = true;
                                                //if (prop.boundingBox != null)
                                                //{
                                                //    if (prop.instances.TryGetValue(uuid, out Prop.Instance? instance))
                                                //    {
                                                //        if (prop.boundingBox.GetScreenSpaceRect(instance.transform, out Rect rect, this.renderer.viewports[0]))
                                                //        {
                                                //            if (rect.height > 10f)
                                                //                prop.cullingResults[uuid] = true;
                                                //        }
                                                //    }
                                                //}
                                            }
                                        }

                                        if (!areaprop.loadRequested)
                                        {
                                            areaprop.loadRequested = true;
                                            if (areaprop.modelType == FileFormats.Area.AreaProp.ModelType.M3)
                                            {
                                                // Load M3 Model //
                                                this.engine.resourceManager.LoadM3Model(areaprop.path);
                                                this.engine.resourceManager.modelResources[areaprop.path].TryBuildObject(areaprop);
                                            }
                                            else if (areaprop.modelType == FileFormats.Area.AreaProp.ModelType.I3)
                                            {
                                                // Load I3 Model //
                                            }
                                            else if (areaprop.modelType == FileFormats.Area.AreaProp.ModelType.DGN)
                                            {
                                                // Load DGN File //
                                            }
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }

        void CreateChunks(List<Vector2i> available, string projectPath, string assetPath, string mapName)
        {
            for (int x = 0; x < WORLD_SIZE; x++)
            {
                for (int y = 0; y < WORLD_SIZE; y++)
                {
                    Vector2i coords = new Vector2i(x, y);

                    if (available.Contains(coords))
                    {
                        string xHex = x.ToString("X2").ToLower();
                        string yHex = y.ToString("X2").ToLower();

                        string path = $"{projectPath}\\{assetPath}\\{mapName}.{yHex}{xHex}.area";

                        // TODO : find which area low file belongs to this chunk and add the file reference to Chunk constructor

                        this.chunks?.Add(coords, new Chunk(coords, path, this));
                    }
                }
            }
        }
    }
}
