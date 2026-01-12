using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace ProjectWS.Engine.TaskManager
{
    public class BuildObjectTask : Task
    {
        Objects.GameObject gameObject;

        public BuildObjectTask(Objects.GameObject gameObject)
        {
            this.gameObject = gameObject;
            this.dataType = DataType.Object;
        }

        public override void PerformTask()
        {
            this.gameObject.Build();
        }
    }
}
