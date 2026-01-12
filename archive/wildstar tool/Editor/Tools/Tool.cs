using MathUtils;
using ProjectWS.Engine.Database.Definitions;
using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace ProjectWS.Editor.Tools
{
    public abstract class Tool
    {
        public bool isEnabled;
        public bool hasBrush;

        public readonly Vector2i[] areaKernel = new Vector2i[]
        {
            new Vector2i(-1, -1), new Vector2i(0, -1), new Vector2i(1, -1),
            new Vector2i(-1,  0), new Vector2i(0,  0), new Vector2i(1,  0),
            new Vector2i(-1,  1), new Vector2i(0,  1), new Vector2i(1,  1)
        };

        public abstract void Enable();
        public abstract void Disable();
        public abstract void Update(float deltaTime);

        public abstract void OnTooboxPaneLoaded();
    }
}
