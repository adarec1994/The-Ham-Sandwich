using MathUtils;
using ProjectWS.Engine.Rendering;
using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace ProjectWS.Engine
{
    public class OrthoCamera : Camera
    {
        public Frustum frustum;
        public Vector2 viewportSize;
        public float zoom = 100.0f;

        public OrthoCamera(Rendering.Renderer renderer, Vector3 position, float nearDistance, float farDistance)
        {
            this.renderer = renderer;
            this.transform.SetPosition(position);
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
            // Calculate the projection matrix
            this.projection = Matrix4.CreateOrthographicOffCenter(
                viewportSize.X / 2 / zoom,  // left
                -viewportSize.X / 2 / zoom,   // right
                viewportSize.Y / 2 / zoom,  // bottom
                -viewportSize.Y / 2 / zoom,   // top
                -1.0f,                       // near
                1.0f);                       // far

            // Update frustum (TODO : Make box instead of frustum for ortho)
            //this.frustum.CalculateFrustum(this.projection, this.view);
        }
    }
}
