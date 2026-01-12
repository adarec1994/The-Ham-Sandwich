using MathUtils;
using ProjectWS.Engine.Components;
using ProjectWS.Engine.Data;
using ProjectWS.Engine.Objects.Gizmos;
using ProjectWS.Engine.Rendering;
using static ProjectWS.Engine.World.Prop;

namespace ProjectWS.Engine
{
    public class MousePick
    {
        public Mode mode = Mode.Disabled;
        private WorldRenderer renderer;
        private Engine engine;
        public Ray mouseRay;

        public Vector3 terrainHitPoint;
        public Vector2i terrainSubchunkHit;
        public Vector2i areaHit;
        public World.Prop? propHit;
        public World.Prop.Instance? propInstanceHit;
        public Vector3 propHitPoint;

        public enum Mode
        {
            Disabled,
            Terrain,
            Prop,
        }

        public MousePick(WorldRenderer renderer, Engine engine)
        {
            this.renderer = renderer;
            this.engine = engine;
        }

        public void Update()
        {
            if (this.renderer.viewports == null)
                return;

            for (int v = 0; v < this.renderer.viewports.Count; v++)
            {
                var vp = this.renderer.viewports[v];

                if (vp.interactive)
                {
                    var mousePos = this.renderer.engine.input.GetMousePosition();
                    this.mouseRay = new Ray(vp.mainCamera.transform.GetPosition(), Unproject(vp, mousePos));
                }
            }

            if (this.mode == Mode.Terrain)
                TerrainPick();
            else if (this.mode == Mode.Prop && this.engine.input.LMBClicked == Input.Input.ClickState.MouseButtonDown)
                PropPick();
        }

        private void TerrainPick()
        {
            // Terrain Pick //
            if (this.renderer.world != null)
            {
                this.renderer.brushParameters.isEnabled = false;
                Vector3[] triPoints = new Vector3[4];

                foreach (var chunkItem in this.renderer.world.chunks)
                {
                    if (chunkItem.Value.area != null && chunkItem.Value.lod0Available && chunkItem.Value.area.subAreas != null)
                    {
                        var areaPos = chunkItem.Value.worldCoords;

                        foreach (var sc in chunkItem.Value.subChunks)
                        {
                            if (sc.isVisible)
                            {
                                Vector2 result = sc.AABB.IntersectsRay(this.mouseRay);

                                if (result.X <= result.Y)
                                {
                                    var subPos = new Vector3(sc.X * 32f, 0f, sc.Y * 32f) + areaPos;

                                    this.terrainSubchunkHit = new Vector2i(sc.X, sc.Y);
                                    this.areaHit = chunkItem.Key;

                                    if (sc.terrainMesh.MeshIntersectsRay(this.mouseRay, subPos, Quaternion.Identity, Vector3.One, ref triPoints))
                                    {
                                        this.renderer.brushParameters.isEnabled = true;
                                        this.terrainHitPoint = triPoints[0];
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }

        /// <summary>
        /// Iterate over all the props in the world, and all instances that are visible
        /// and check if the mouse ray intersects with the bounding boxes, then pick the most suitable prop to be selected
        /// </summary>
        private void PropPick()
        {
            if (this.renderer.world != null)
            {
                this.renderer.brushParameters.isEnabled = false;
                Vector3[] triPoints = new Vector3[4];

                float propMinDist = float.MaxValue;
                int instanceIndex = 0;
                bool hit = false;

                foreach (var propItem in this.renderer.world.props)
                {
                    //var aabb = propItem.Value.aabb;

                    float instanceMinDist = float.MaxValue;
                    int instanceCounter = 0;
                    foreach (var instanceItem in propItem.Value.instances)
                    {
                        var instance = instanceItem.Value;
                        if (instance.visible)
                        {
                            Vector2 intersection = instance.obb.IntersectsRay(this.mouseRay, instance.position, instance.scale);
                            if (intersection.X <= intersection.Y)
                            {
                                float meshMinDist = float.MaxValue;
                                if (propItem.Value.hasMeshes)
                                {

                                    for (int g = 0; g < propItem.Value.geometries?.Length; g++)
                                    {
                                        var geometry = propItem.Value.geometries[g];
                                        for (int m = 0; m < geometry?.meshes?.Length; m++)
                                        {
                                            // Mesh pick
                                            if (geometry.meshes == null || geometry.meshes[m] == null) continue;

                                            if (geometry.meshes[m].MeshIntersectsRay(this.mouseRay, instance.position, instance.rotation, instance.scale, ref triPoints))
                                            {
                                                for (int i = 0; i < 4; i++)
                                                {
                                                    var dist = Vector3.DistanceSquared(triPoints[i], this.mouseRay.origin);
                                                    if (dist < meshMinDist)
                                                    {
                                                        meshMinDist = dist;
                                                        this.propHitPoint = triPoints[i];
                                                        hit = true;
                                                    }
                                                }
                                            }
                                        }
                                    }
                                }
                                else
                                {
                                    // Icon pick

                                    // Define the sphere
                                    Vector3 center = instance.position;
                                    float radius = 2.5f;

                                    // Calculate the vector from the origin of the ray to the center of the sphere
                                    Vector3 oc = center - this.mouseRay.origin;

                                    // Calculate the projection of oc onto the direction of the ray
                                    float t = Vector3.Dot(oc, this.mouseRay.direction);

                                    // Calculate the distance between the center of the sphere and the projection
                                    float d = (oc - t * this.mouseRay.direction).Length;

                                    // Check for intersection
                                    if (d <= radius)
                                    {
                                        this.propHitPoint = instance.position;
                                        this.propInstanceHit = instance;
                                        this.propHit = propItem.Value;

                                        return; // Icons have hit priority
                                    }
                                }

                                if (meshMinDist < instanceMinDist)
                                {
                                    instanceMinDist = meshMinDist;
                                    this.propInstanceHit = instance;
                                }
                            }
                        }

                        instanceCounter++;
                    }

                    if (instanceMinDist < propMinDist)
                    {
                        propMinDist = instanceMinDist;
                        this.propHit = propItem.Value;
                        instanceIndex = instanceCounter;
                    }
                }

                if (!hit)
                {
                    this.propHit = null;
                    this.propInstanceHit = null;
                }
            }
        }

        private void DrawOBB(OBB obb, Vector3 position, Quaternion rotation, Vector3 scale, Color color)
        {
            var min = obb.center - (obb.size * 0.5f);
            Debug.DrawWireBox3D(position + (obb.center / scale), rotation, scale * obb.size, color);
        }

        private Vector3 Unproject(Viewport vp, Vector3 mousePos)
        {
            var ndc = MouseToNormalizedDeviceCoords(mousePos, vp.width, vp.height);
            var clip = NDCToClipCoords(ndc);

            //var projection = Matrix4.CreatePerspectiveFieldOfView(
            //    vp.mainCamera.fov, vp.mainCamera.aspectRatio, vp.mainCamera.nearDistance, vp.mainCamera.farDistance);

            var eye = ClipToEye(clip, vp.mainCamera.projection);

            var cc = vp.mainCamera.components[0] as CameraController;

            var rayvec = EyeToRayVector(eye, vp.mainCamera.view);
            return rayvec;
        }

        Vector2 MouseToNormalizedDeviceCoords(Vector3 mousePos, int width, int height)
        {
            float x = (2.0f * mousePos.X) / width - 1.0f;
            float y = 1.0f - (2.0f * mousePos.Y) / height;
            return new Vector2(x, y);
        }

        Vector4 NDCToClipCoords(Vector2 ray_nds)
        {
            return new Vector4(ray_nds.X, ray_nds.Y, -1.0f, 1.0f);
        }

        Vector4 ClipToEye(Vector4 ray_clip, Matrix4 projection_matrix)
        {
            Vector4 ray_eye = ray_clip * projection_matrix.Inverted();
            return new Vector4(ray_eye.X, ray_eye.Y, -1.0f, 0.0f);
        }

        Vector3 EyeToRayVector(Vector4 ray_eye, Matrix4 view_matrix)
        {
            Vector3 ray_wor = (ray_eye * view_matrix.Inverted()).Xyz;
            ray_wor.Normalize();

            return ray_wor;
        }

        Vector3 GetPointOnRay(Vector3 ray, float distance, Vector3 camPos)
        {
            Vector3 start = new Vector3(camPos.X, camPos.Y, camPos.Z);
            Vector3 scaledRay = new Vector3(ray.X * distance, ray.Y * distance, ray.Z * distance);
            return start + scaledRay;
        }

        void DrawAABB(AABB aabb, Vector3 position, Vector3 scale, Color color)
        {
            var positionOffsetMat = Matrix4.CreateTranslation(new Vector3(position.X, aabb.center.Y + position.Y, position.Z));
            var scaleMat = Matrix4.CreateScale(aabb.size * scale);
            var boxMat = scaleMat * positionOffsetMat;

            Debug.DrawWireBox3D(boxMat, color);
        }

        internal void DrawOBB(OBB obb, Matrix4 transform, Color color)
        {
            var positionOffsetMat = Matrix4.CreateTranslation(obb.center);
            var scaleMat = Matrix4.CreateScale(obb.size);
            var boxMat = scaleMat * positionOffsetMat * transform;

            Debug.DrawWireBox3D(boxMat, color);
        }
    }
}
