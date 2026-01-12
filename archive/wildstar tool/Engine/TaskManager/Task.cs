using System.Collections;
using System.Collections.Generic;

namespace ProjectWS.Engine.TaskManager
{
    public class Task
    {
        public DataType dataType;
        public JobType jobType;

        public virtual void PerformTask() { }

        public enum DataType
        {
            Archive,
            Database,
            Terrain,
            Texture,
            Object,
        }

        public enum JobType
        {
            Read,       // Separate thread
            Write,      // Separate thread
            Build,      // Main thread
            Destroy,    // Main thread
        }
    }
}