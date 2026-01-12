using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace ProjectWS.Engine.Components
{
    public abstract class Component
    {
        public abstract void Update(float deltaTime);
    }
}
