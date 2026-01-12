using ProjectWS.Engine.Data;
using System;
using System.Collections;
using System.Collections.Generic;
using System.Diagnostics;
using System.IO;

namespace ProjectWS.Engine.TaskManager
{
    public class ModelTask : Task
    {
        string filePath;
        Data.ResourceManager.Manager resourceManager;

        public ModelTask(string filePath, JobType jobType, Data.ResourceManager.Manager resourceManager)
        {
            this.filePath = filePath;
            this.dataType = DataType.Texture;
            this.jobType = jobType;
            this.resourceManager = resourceManager;
        }

        public override void PerformTask()
        {
            switch (this.jobType)
            {
                case JobType.Read:
                    this.resourceManager.modelResources[this.filePath].SetFileState(Data.ResourceManager.Manager.ResourceState.IsLoading);
                    using (Stream str = DataManager.GetFileStream(this.filePath))
                    {
                        this.resourceManager.modelResources[this.filePath].m3.Read(str);
                    }
                    this.resourceManager.modelResources[this.filePath].SetFileState(Data.ResourceManager.Manager.ResourceState.IsLoaded);
                    this.jobType = JobType.Build;
                    this.resourceManager.engine.taskManager.buildTasks.Enqueue(this);
                    break;
                case JobType.Write:
                    break;
                case JobType.Build:
                    this.resourceManager.modelResources[this.filePath].BuildAllRefs();
                    this.resourceManager.modelResources[this.filePath].SetFileState(Data.ResourceManager.Manager.ResourceState.IsReady);
                    break;
                case JobType.Destroy:
                    break;
                default:
                    break;
            }
        }
    }
}