using System.Runtime.CompilerServices;

namespace MathUtils
{
    public class AABB
    {
        public Vector3 min;
        public Vector3 max;
        public Vector3 center;
        public Vector3 extents;
        public Vector3 size;

        public Matrix4 boxMat;      // A matrix that when applied to a 1,1,1 box will transform it into this aabb
                                    // Used strictly for rendering the AABB for debug
        public bool boxMatNeedsUpdate;

        public AABB(Vector3 center, Vector3 extents)
        {
            this.center = center;
            this.extents = extents;
            this.size = this.extents * 2;
            this.min = center - extents;
            this.max = center + extents;
            this.boxMatNeedsUpdate = true;
        }

        public AABB(BinaryReader br)
        {
            this.min = new Vector3(br.ReadSingle(), br.ReadSingle(), br.ReadSingle()); br.ReadSingle(); // skip W
            this.max = new Vector3(br.ReadSingle(), br.ReadSingle(), br.ReadSingle()); br.ReadSingle(); // skip W
            this.center = (this.min + this.max) / 2;
            this.extents = (this.max - this.min) / 2;
            this.size = this.max - this.min;
            this.boxMatNeedsUpdate = true;
        }

        public AABB(AABB bounds)
        {
            this.min = bounds.min;
            this.max = bounds.max;
            this.center = bounds.center;
            this.extents = bounds.extents;
            this.size = bounds.size;
            this.boxMatNeedsUpdate = true;
        }

        public Vector3[] GetCorners()
        {
            return new Vector3[]
            {
                new Vector3(center.X + extents.X, center.Y - extents.Y, center.Z - extents.Z),
                new Vector3(center.X - extents.X, center.Y - extents.Y, center.Z + extents.Z),
                new Vector3(center.X + extents.X, center.Y - extents.Y, center.Z + extents.Z),
                new Vector3(center.X - extents.X, center.Y + extents.Y, center.Z - extents.Z),
                new Vector3(center.X + extents.X, center.Y + extents.Y, center.Z - extents.Z),
                new Vector3(center.X - extents.X, center.Y + extents.Y, center.Z + extents.Z),
                new Vector3(center.X + extents.X, center.Y + extents.Y, center.Z + extents.Z)
            };
        }

        public bool PointInOABB(Vector3 point)
        {
            return PointInOABB(point, Vector3.Zero);
        }

        public bool PointInOABB(Vector3 point, Vector3 space)
        {
            point -= space;
            point -= this.center;

            if (point.X < this.extents.X && point.X > -this.extents.X &&
               point.Y < this.extents.Y && point.Y > -this.extents.Y &&
               point.Z < this.extents.Z && point.Z > -this.extents.Z)
                return true;
            else
                return false;
        }

        /*
        /// <summary>
        /// Calculate the screen coordinates of the bounding box
        /// </summary>
        /// <returns> topRight, topLeft, bottomRight, bottomLeft </returns>
        public Vector3[] GetScreenSpaceBounds(Matrix4x4 V, Matrix4x4 P, Vector3 offset)
        {
            Vector3 c = center;
            Vector3 e = extents;

            Vector3[] worldCorners = new[] {
                    new Vector3( c.X + e.X, c.Y + e.Y, c.Z + e.Z ),
                    new Vector3( c.X + e.X, c.Y + e.Y, c.Z - e.Z ),
                    new Vector3( c.X + e.X, c.Y - e.Y, c.Z + e.Z ),
                    new Vector3( c.X + e.X, c.Y - e.Y, c.Z - e.Z ),
                    new Vector3( c.X - e.X, c.Y + e.Y, c.Z + e.Z ),
                    new Vector3( c.X - e.X, c.Y + e.Y, c.Z - e.Z ),
                    new Vector3( c.X - e.X, c.Y - e.Y, c.Z + e.Z ),
                    new Vector3( c.X - e.X, c.Y - e.Y, c.Z - e.Z ),
                };

            IEnumerable<Vector3> screenCorners = worldCorners.Select(corner => Camera.main.WorldToScreenPoint(corner));
            float maxX = screenCorners.Max(corner => corner.x);
            float minX = screenCorners.Min(corner => corner.x);
            float maxY = screenCorners.Max(corner => corner.y);
            float minY = screenCorners.Min(corner => corner.y);

            Vector3 topRight = new Vector3(maxX, maxY, 0);
            Vector3 topLeft = new Vector3(minX, maxY, 0);
            Vector3 bottomRight = new Vector3(maxX, minY, 0);
            Vector3 bottomLeft = new Vector3(minX, minY, 0);

            return new Vector3[] { topRight, topLeft, bottomRight, bottomLeft };
        }
        */

        public bool GetScreenSpaceRect(Matrix4 transform, out Rect result, Matrix4 V, Matrix4 P, int width, int height)
        {
            // Transform bounds to world space //
            Vector3 scale = transform.ExtractScale();
            Vector3 size = this.size;
            Vector3 center = this.center;
            center *= scale;
            size *= scale;

            Vector3 cen = center + transform.ExtractPosition();
            Vector3 ext = extents;
            ext *= transform.ExtractScale();

            // Likely need to rotate each ext around cen based on transform.GetRotation()

            //Camera cam = Camera.main;
            //Matrix4 V = vp.mainCamera.view;
            //Matrix4 P = vp.mainCamera.projection;
            Matrix4 VP = P * V;

            Vector2 min = WorldToScreenPoint(new Vector3(cen.X - ext.X, cen.Y - ext.Y, cen.Z - ext.Z), VP, width, height).Xy;
            Vector2 max = min;
            result = new Rect(0, 0, 0, 0);

            //0
            Vector3 point = new Vector3(min.X, min.Y, 0);
            get_minMax(point.Xy, ref min, ref max);

            //1
            point = WorldToScreenPoint(new Vector3(cen.X + ext.X, cen.Y - ext.Y, cen.Z - ext.Z), VP, width, height);
            //if (point.Z > 1) return false;
            get_minMax(point.Xy, ref min, ref max);


            //2
            point = WorldToScreenPoint(new Vector3(cen.X - ext.X, cen.Y - ext.Y, cen.Z + ext.Z), VP, width, height);
            //if (point.Z > 1) return false;
            get_minMax(point.Xy, ref min, ref max);

            //3
            point = WorldToScreenPoint(new Vector3(cen.X + ext.X, cen.Y - ext.Y, cen.Z + ext.Z), VP, width, height);
            //if (point.Z > 1) return false;
            get_minMax(point.Xy, ref min, ref max);

            //4
            point = WorldToScreenPoint(new Vector3(cen.X - ext.X, cen.Y + ext.Y, cen.Z - ext.Z), VP, width, height);
            //if (point.Z > 1) return false;
            get_minMax(point.Xy, ref min, ref max);

            //5
            point = WorldToScreenPoint(new Vector3(cen.X + ext.X, cen.Y + ext.Y, cen.Z - ext.Z), VP, width, height);
            //if (point.Z > 1) return false;
            get_minMax(point.Xy, ref min, ref max);

            //6
            point = WorldToScreenPoint(new Vector3(cen.X - ext.X, cen.Y + ext.Y, cen.Z + ext.Z), VP, width, height);
            //if (point.Z > 1) return false;
            get_minMax(point.Xy, ref min, ref max);

            //7
            point = WorldToScreenPoint(new Vector3(cen.X + ext.X, cen.Y + ext.Y, cen.Z + ext.Z), VP, width, height);
            //if (point.Z > 1) return false;
            get_minMax(point.Xy, ref min, ref max);

            result = new Rect(min.X, min.Y, max.X - min.X, max.Y - min.Y);

            return true;
        }

        Vector3 WorldToScreenPoint(Vector3 point, Matrix4 VP, int width, int height)
        {
            // Main thread, just use this
            //return Camera.main.WorldToScreenPoint(point);

            // Secondary thread, use this
            Vector4 v = VP * new Vector4(point.X, point.Y, point.Z, 1);
            Vector3 viewportPoint = v.Xyz / -v.W;

            // Will keep _ViewPort Under 1 for multiplication.
            Vector3 normalizedViewportPoint = new Vector3(viewportPoint.X + 1, viewportPoint.Y + 1, viewportPoint.Z + 1);
            normalizedViewportPoint /= 2;

            normalizedViewportPoint = Vector3.One - normalizedViewportPoint;

            Vector3 screenPoint;
            screenPoint.X = normalizedViewportPoint.X * width;
            screenPoint.Y = normalizedViewportPoint.Y * height;
            screenPoint.Z = normalizedViewportPoint.Z;

            return screenPoint;
        }

        /*
        public void RenderBounds(Matrix4x4 transform, Color color)
        {
            // Transform bounds to world space //
            Vector3 scale = transform.ExtractScale();
            Vector3 size = this.size;
            Vector3 center = this.center;
            center.Scale(scale);
            size.Scale(scale);
            Vector3 position = transform.ExtractPosition() + (Vector3.up * center.y);
            Quaternion rotation = transform.ExtractRotation();

            IMDraw.WireBox3D(position, rotation, size, color);
        }
        */

        public Vector2 IntersectsRay(Ray ray)
        {
            Vector3 tMin = (this.min - ray.origin) / ray.direction;
            Vector3 tMax = (this.max - ray.origin) / ray.direction;
            Vector3 t1 = new Vector3(MathF.Min(tMin.X, tMax.X), MathF.Min(tMin.Y, tMax.Y), MathF.Min(tMin.Z, tMax.Z));
            Vector3 t2 = new Vector3(MathF.Max(tMin.X, tMax.X), MathF.Max(tMin.Y, tMax.Y), MathF.Max(tMin.Z, tMax.Z));
            float tNear = MathF.Max(MathF.Max(t1.X, t1.Y), t1.Z);
            float tFar = MathF.Min(MathF.Min(t2.X, t2.Y), t2.Z);
            return new Vector2(tNear, tFar);
        }

        public Vector2 IntersectsRay(Ray ray, Vector3 worldPosition)
        {
            Vector3 tMin = this.min + worldPosition;
            Vector3 tMax = this.max + worldPosition;

            tMin = (tMin - ray.origin) / ray.direction;
            tMax = (tMax - ray.origin) / ray.direction;
            Vector3 t1 = new Vector3(MathF.Min(tMin.X, tMax.X), MathF.Min(tMin.Y, tMax.Y), MathF.Min(tMin.Z, tMax.Z));
            Vector3 t2 = new Vector3(MathF.Max(tMin.X, tMax.X), MathF.Max(tMin.Y, tMax.Y), MathF.Max(tMin.Z, tMax.Z));
            float tNear = MathF.Max(MathF.Max(t1.X, t1.Y), t1.Z);
            float tFar = MathF.Min(MathF.Min(t2.X, t2.Y), t2.Z);
            return new Vector2(tNear, tFar);
        }

        /*
        /// <summary>
        /// Check if ray intersects bounds
        /// This is faster than unity *.bounds.IntersectRay(ray)
        /// Original source https://gamedev.stackexchange.com/a/103714/73429
        /// </summary>
        /// <param name="ray">Ray to check</param>
        /// <returns>Intersection distance, if > 0 it intersects</returns>
        public Vector2 RayBoxIntersect(Vector3 rayOrigin, Vector3 rayDir, Matrix4 transform)
        {
            Vector3 scale = transform.ExtractScale();
            Vector3 tMin = this.min;
            Vector3 tMax = this.max;
            tMin *= scale;
            tMax *= scale;
            tMin += transform.ExtractPosition();
            tMax += transform.ExtractPosition();

            tMin = (tMin - rayOrigin) / rayDir;
            tMax = (tMin - rayOrigin) / rayDir;
            Vector3 t1 = new Vector3(MathF.Min(tMin.X, tMax.X), MathF.Min(tMin.Y, tMax.Y), MathF.Min(tMin.Z, tMax.Z));
            Vector3 t2 = new Vector3(MathF.Max(tMin.X, tMax.X), MathF.Max(tMin.Y, tMax.Y), MathF.Max(tMin.Z, tMax.Z));
            float tNear = MathF.Max(MathF.Max(t1.X, t1.Y), t1.Z);
            float tFar = MathF.Min(MathF.Min(t2.X, t2.Y), t2.Z);
            return new Vector2(tNear, tFar);
        */
            /*
            Vector3 rpos = rayOrigin;
            Vector3 rdir = rayDir;

            float t1 = (tMin.X - rpos.X) / rdir.X;
            float t2 = (tMax.X - rpos.X) / rdir.X;
            float t3 = (tMin.Y - rpos.Y) / rdir.Y;
            float t4 = (tMax.Y - rpos.Y) / rdir.Y;
            float t5 = (tMin.Z - rpos.Z) / rdir.Z;
            float t6 = (tMax.Z - rpos.Z) / rdir.Z;

            float aMin = t1 < t2 ? t1 : t2;
            float bMin = t3 < t4 ? t3 : t4;
            float cMin = t5 < t6 ? t5 : t6;

            float aMax = t1 > t2 ? t1 : t2;
            float bMax = t3 > t4 ? t3 : t4;
            float cMax = t5 > t6 ? t5 : t6;

            float fMax = aMin > bMin ? aMin : bMin;
            float fMin = aMax < bMax ? aMax : bMax;

            float t7 = fMax > cMin ? fMax : cMin;
            float t8 = fMin < cMax ? fMin : cMax;

            float t9 = (t8 < 0 || t7 > t8) ? -1 : t7;

            return t9;
            */
       //}

        /*
        public void RenderScreenBounds(Matrix4x4 transform, Color color, Matrix4x4 V, Matrix4x4 P)
        {
            if (GetScreenSpaceRect(transform, out Rect rect, V, P))
            {
                rect.Y = (Screen.height - rect.y) - rect.height;
                IMDraw.RectangleFilled2D(rect, color);
            }
        }
        */

        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        void get_minMax(Vector2 point, ref Vector2 min, ref Vector2 max)
        {
            min = new Vector2(min.X >= point.X ? point.X : min.X, min.Y >= point.Y ? point.Y : min.Y);
            max = new Vector2(max.X <= point.X ? point.X : max.X, max.Y <= point.Y ? point.Y : max.Y);
        }
        /*
Vector3 WorldToScreenPoint(Vector3 point, Matrix4x4 V, Matrix4x4 P, Vector3 offset)
{
   Matrix4 MVP = P * V;
   Vector3 screenPos = MVP.MultiplyPoint(point + offset);
   return new Vector3(screenPos.X + 1f, screenPos.Y + 1f, screenPos.Z + 1f) / 2f;
}
*/
    }
}