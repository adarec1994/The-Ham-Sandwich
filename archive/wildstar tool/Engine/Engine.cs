using MathUtils;
using OpenTK.Graphics.OpenGL4;
using OpenTK.Windowing.GraphicsLibraryFramework;
using ProjectWS.Engine.Data;

namespace ProjectWS.Engine
{
    /// <summary>
    /// Entry point class for the project
    /// Extends MonoBehaviour so that it can be attached to a game object in the unity scene
    /// This is required in unity to gain access to its Start and Update methods
    /// Only one instance of Engine must exist, and this instance is created automatically when Unity runs
    /// Engine holds reference to all of the classes, and is used so, to keep the number of static things close to 0
    /// </summary>
    public class Engine
    {
        public static Settings? settings = new Settings();
        bool running = false;
        public bool contextAvailable = false;
        public float deltaTime;
        public float frameTime;
        public float time;
        public int focusedRendererID;
        public int total_mem_kb;
        public int total_mem_mb;
        public int cur_avail_mem_kb;
        public int initial_avail_mem_kb;
        public int memory_usage_kb;
        public int memory_usage_mb;
        public bool firstFrame = true;

        public TaskManager.Manager? taskManager;
        public Data.ResourceManager.Manager? resourceManager;
        public World.World? world;
        public World.World? unloadWorld;
        public Data.GameData? data;
        public Input.Input? input;
        public Dictionary<uint, FileFormats.Sky.File>? skyData;
        public Dictionary<int, Rendering.Renderer>? renderers;

        /// <summary>
        /// Code to be executed on application launch
        /// </summary>
        public Engine()
        {
            DataManager.engine = this;
            SettingsSerializer.Load();
            this.taskManager = new TaskManager.Manager(this);
            this.resourceManager = new Data.ResourceManager.Manager(this);
            this.renderers = new Dictionary<int, Rendering.Renderer>();
            this.input = new Input.Input(this);
            this.skyData = new Dictionary<uint, FileFormats.Sky.File>();
            this.running = true;
        }
        /*
        public void LoadGameData(string installLocation, Action<Data.GameData> onLoaded)
        {
            this.data = new Data.GameData(this, installLocation, onLoaded);
            this.taskManager.otherThread.Enqueue(new TaskManager.ArchiveTask(this.data, TaskManager.Task.JobType.Read, this.taskManager));
        }
        */
        /// <summary>
        /// Code to be execute every frame
        /// </summary>
        public void Update(float deltaTime, float timeScale)
        {
            if (!this.running) return;

            this.taskManager?.Update();
            this.input?.Update();

            if (this.input.GetKeyPress(Keys.R))
            {
                for (int i = 0; i < this.renderers?.Count; i++)
                {
                    Debug.Log($"Reload Shaders: Renderer[{i}]");
                    this.renderers[i].normalShader?.Load();
                    this.renderers[i].modelShader?.Load();
                    this.renderers[i].wireframeShader?.Load();
                    this.renderers[i].terrainShader?.Load();
                    this.renderers[i].infiniteGridShader?.Load();
                    this.renderers[i].waterShader?.Load();
                    this.renderers[i].fontShader?.Load();
                    this.renderers[i].iconShader?.Load();
                    this.renderers[i].lightPassShader?.Load();
                    this.renderers[i].mapTileShader?.Load();
                    this.renderers[i].marqueeShader?.Load();
                }
            }

            if (this.firstFrame)
            {
                // Save total mem
                this.total_mem_kb = GL.GetInteger((GetPName)GL_GPU_MEM_INFO_TOTAL_AVAILABLE_MEM_NVX);
                this.total_mem_mb = (int)((float)this.total_mem_kb / 1024f);

                // Save gpu mem
                this.initial_avail_mem_kb = GL.GetInteger((GetPName)GL_GPU_MEM_INFO_CURRENT_AVAILABLE_MEM_NVX);
                this.firstFrame = false;
            }

            //for (int i = 0; i < this.renderers.Count; i++)
            foreach (var rendererItem in this.renderers)
            {
                //this.renderers[i].Update(deltaTime);
                rendererItem.Value.Update(deltaTime);
            }

            this.deltaTime = deltaTime;
            this.time += this.deltaTime;
            this.frameTime += ((deltaTime / timeScale) - this.frameTime) * 0.03f;

            CalculateGPUMemory();
        }

        const int GL_GPU_MEM_INFO_TOTAL_AVAILABLE_MEM_NVX = 0x9048;
        const int GL_GPU_MEM_INFO_CURRENT_AVAILABLE_MEM_NVX = 0x9049;

        void CalculateGPUMemory()
        {
            this.cur_avail_mem_kb = GL.GetInteger((GetPName)GL_GPU_MEM_INFO_CURRENT_AVAILABLE_MEM_NVX);
            this.memory_usage_kb = (this.total_mem_kb - this.cur_avail_mem_kb) - (this.total_mem_kb - this.initial_avail_mem_kb);
            this.memory_usage_mb = (int)((float)this.memory_usage_kb / 1024f);
        }

        public void Render(int renderer, int frameBuffer)
        {
            this.contextAvailable = true;
            this.renderers[renderer].Render(frameBuffer);

            GL.Flush();
        }

        ~Engine()
        {
            for (int i = 0; i < this.renderers.Count; i++)
            {
                if (this.renderers == null) break;
                if (this.renderers[i] == null) continue;
                this.renderers[i].rendering = false;
            }
            
            if (this.taskManager != null)
                this.taskManager.Destructor();
        }

        public FileFormats.Sky.File GetSky(uint ID)
        {
            if (this.skyData.ContainsKey(ID))
                return this.skyData[ID];

            var worldSkyRecord = this.data.database.worldSky.Get(ID);
            FileFormats.Sky.File sky = new FileFormats.Sky.File(worldSkyRecord.assetPath);
            sky.Read(this.data.GetFileData(worldSkyRecord.assetPath));
            this.skyData.Add(ID, sky);
            return sky;
        }

        public void LoadWorld(uint iD, string projectFolder, string? assetPath, string? name, List<Vector2i>? chunks, Vector3 position)
        {
            if (this.world != null)
            {
                this.unloadWorld = this.world;
                this.unloadWorld.UnloadMap();
            }

            this.world = new World.World(this);
            this.world.LoadMap(iD, projectFolder, assetPath, name, chunks, position);
        }
    }
}