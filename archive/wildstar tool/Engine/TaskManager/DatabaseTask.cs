using System.Collections;
using System.Collections.Generic;

namespace ProjectWS.Engine.TaskManager
{
    public class DatabaseTask : Task
    {
        public Database.Tables tables;
        TaskManager.Manager taskManager;

        public DatabaseTask(Database.Tables tables, JobType jobType, TaskManager.Manager taskManager)
        {
            this.dataType = DataType.Archive;
            this.tables = tables;
            this.jobType = jobType;
            this.taskManager = taskManager;
        }

        public override void PerformTask()
        {
            switch (this.jobType)
            {
                case JobType.Read:
                    this.tables.Read();
                    this.jobType = JobType.Build;
                    this.taskManager.buildTasks.Enqueue(this);
                    break;
                case JobType.Write:
                    break;
                case JobType.Build:
                    if (this.tables.data.onLoaded != null)
                        this.tables.data.onLoaded(this.tables.data);
                    break;
                case JobType.Destroy:
                    break;
                default:
                    break;
            }
        }
    }
}