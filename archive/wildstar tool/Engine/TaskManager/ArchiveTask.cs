using System;
using System.Collections;
using System.Collections.Generic;

namespace ProjectWS.Engine.TaskManager
{
    public class ArchiveTask : Task
    {
        public Data.GameData data;
        TaskManager.Manager taskManager;

        public ArchiveTask(Data.GameData data, JobType jobType, TaskManager.Manager taskManager)
        {
            this.dataType = DataType.Archive;
            this.data = data;
            this.jobType = jobType;
            this.taskManager = taskManager;
        }

        public override void PerformTask()
        {
            switch (this.jobType)
            {
                case JobType.Read:
                    this.data.Read();
                    this.jobType = JobType.Build;
                    this.taskManager.buildTasks.Enqueue(this);
                    break;
                case JobType.Write:
                    break;
                case JobType.Build:
                    //this.data.onLoaded(this.data); // moved to on database loaded
                    break;
                case JobType.Destroy:
                    break;
                default:
                    break;
            }
        }
    }
}