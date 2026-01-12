using MathUtils;
using ProjectWS.Engine.Rendering;
using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace ProjectWS.Engine
{
    public class PerspectiveCamera : Camera
    {
        public Frustum frustum;

        public PerspectiveCamera(Rendering.Renderer renderer, Vector3 position, float fov, float aspectRatio, float nearDistance, float farDistance)
        {
            this.renderer = renderer;
            this.transform.SetPosition(position);
            this.fov = fov;
            this.aspectRatio = aspectRatio;
            this.nearDistance = nearDistance;
            this.farDistance = farDistance;
            this.frustum = new Frustum();
            this.gizmo = new Objects.Gizmos.CameraGizmo(this);
            if (this.renderer.gizmos != null)
                this.renderer.gizmos.Add(this.gizmo);
            if (this.renderer.engine != null)
                this.renderer.engine.taskManager.buildTasks.Enqueue(new TaskManager.BuildObjectTask(this.gizmo));
        }

        public override void Update(float deltaTime)
        {
            this.projection = Matrix4.CreatePerspectiveFieldOfView(this.fov, this.aspectRatio, this.nearDistance, this.farDistance);

            // Update frustum
            this.frustum.CalculateFrustum(this.projection, this.view);
        }
    }
}
