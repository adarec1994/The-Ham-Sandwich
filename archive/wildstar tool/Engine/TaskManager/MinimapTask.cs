using ProjectWS.Engine.World;
using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace ProjectWS.Engine.TaskManager
{
    public class MinimapTask : Task
    {
        MinimapChunk chunk;

        public MinimapTask(MinimapChunk chunk)
        {
            this.dataType = DataType.Texture;
            this.chunk = chunk;
        }

        public override void PerformTask()
        {
            switch (this.jobType)
            {
                case JobType.Read:
                        this.chunk.Read();
                    break;
                case JobType.Write:
                    break;
                case JobType.Build:
                    break;
                case JobType.Destroy:
                    break;
                default:
                    break;
            }
        }
    }
}
