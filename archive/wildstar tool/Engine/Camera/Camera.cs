using OpenTK;
using MathUtils;
using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace ProjectWS.Engine
{
    public abstract class Camera : Objects.GameObject
    {
        public Objects.Gizmos.CameraGizmo? gizmo;
        public Rendering.Renderer? renderer;
        public float fov;
        public float aspectRatio;
        public float nearDistance;
        public float farDistance;
        public Matrix4 projection;
        public Matrix4 view;

        public void SetToShader(Shader shader)
        {
            if (this.aspectRatio == 0)
                this.aspectRatio = 1;

            shader.SetMat4("projection", ref this.projection);
            shader.SetMat4("view", ref this.view);
            shader.SetVec3("viewPos", this.transform.GetPosition());
        }
    }
}
