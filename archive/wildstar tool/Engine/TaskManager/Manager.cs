using System;
using System.Collections;
using System.Collections.Concurrent;
using System.Collections.Generic;
using System.Threading;

namespace ProjectWS.Engine.TaskManager
{
    public class Manager
    {
        Engine engine;
        public TThread otherThread;
        public TThread terrainThread;
        public TThread textureThread;
        public TThread modelThread;

        public ConcurrentQueue<Task> buildTasks = new ConcurrentQueue<Task>();

        public Manager(Engine engine)
        {
            this.engine = engine;
            this.otherThread = new TThread("Other");
            this.terrainThread = new TThread("Terrain");
            this.textureThread = new TThread("Texture");
            this.modelThread = new TThread("Model");
        }

        public void Update()
        {
            if (!this.otherThread.isRunning)
            {
                if (this.otherThread.tasks.Count > 0)
                {
                    this.otherThread.Boot(1000);
                }
            }

            if (!this.terrainThread.isRunning)
            {
                if (this.terrainThread.tasks.Count > 0)
                {
                    this.terrainThread.Boot(0);
                }
            }

            if (!this.textureThread.isRunning)
            {
                if (this.textureThread.tasks.Count > 0)
                {
                    this.textureThread.Boot(0);
                }
            }

            if (!this.modelThread.isRunning)
            {
                if (this.modelThread.tasks.Count > 0)
                {
                    this.modelThread.Boot(0);
                }
            }

            if (this.buildTasks.Count > 0)
            {
                if (this.engine.contextAvailable)
                {
                    if (this.buildTasks.TryDequeue(out Task? task))
                    {
                        if (task != null)
                            task.PerformTask();
                    }
                }
            }
        }

        public void Destructor()
        {
            if (otherThread != null)
                otherThread.Clear();

            if (terrainThread != null)
                terrainThread.Clear();

            if (textureThread != null)
                textureThread.Clear();

            if (modelThread != null)
                modelThread.Clear();
        }
    }
}
