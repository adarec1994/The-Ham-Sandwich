using OpenTK;
using MathUtils;
using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace ProjectWS.Engine.Objects
{
    public class GameObject
    {
        public bool visible = true;
        public Components.Transform transform;
        public List<Components.Component> components;

        public GameObject()
        { 
            components = new List<Components.Component>();
            transform = new Components.Transform();
        }

        public virtual void Build() { }
        public virtual void Update(float deltaTime) { }
        public virtual void Render(Matrix4 model, Shader shader) { }
    }
}
