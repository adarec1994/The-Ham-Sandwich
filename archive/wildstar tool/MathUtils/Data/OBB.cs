using MathUtils;
using System.Linq.Expressions;

namespace MathUtils
{
    public class OBB
    {
        public Vector3 center;
        public Quaternion orientation;
        public Vector3 size;

        public Matrix4 boxMat;      // A matrix that when applied to a 1,1,1 box will transform it into this obb
                                    // Used strictly for rendering the OBB for debug
        public bool boxMatNeedsUpdate;

        public OBB(Vector3 center, Quaternion orientation, Vector3 halfSizes)
        {
            this.center = center;
            this.orientation = orientation;
            this.size = halfSizes;

            this.boxMat = Matrix4.Identity;
            this.boxMatNeedsUpdate = true;
        }

        public OBB(AABB aabb, Quaternion orientation)
        {
            this.center = aabb.center;
            this.orientation = orientation;
            this.size = aabb.size;

            this.boxMat = Matrix4.Identity;
            this.boxMatNeedsUpdate = true;
        }

        public Vector2 IntersectsRay(Ray ray, Vector3 worldPosition, Vector3 scale)
        {
            // Transform the ray into the local space of the OBB
            Vector3 rayOrigin = ray.origin - worldPosition;
            Vector3 rayDirection = ray.direction;
            Quaternion inverseOrientation = Quaternion.Invert(this.orientation);
            rayOrigin = inverseOrientation * rayOrigin;
            rayDirection = inverseOrientation * rayDirection;
            rayOrigin /= scale;

            var min = (this.center - ((this.size) * 0.5f));
            var max = (this.center + ((this.size) * 0.5f));

            Vector3 tMin = (min - rayOrigin) / rayDirection;
            Vector3 tMax = (max - rayOrigin) / rayDirection;
            Vector3 t1 = new Vector3(MathF.Min(tMin.X, tMax.X), MathF.Min(tMin.Y, tMax.Y), MathF.Min(tMin.Z, tMax.Z));
            Vector3 t2 = new Vector3(MathF.Max(tMin.X, tMax.X), MathF.Max(tMin.Y, tMax.Y), MathF.Max(tMin.Z, tMax.Z));
            float tNear = MathF.Max(MathF.Max(t1.X, t1.Y), t1.Z);
            float tFar = MathF.Min(MathF.Min(t2.X, t2.Y), t2.Z);
            return new Vector2(tNear, tFar);
        }

        Vector3[] cornerBuffer = new Vector3[8];
        public AABB GetEncapsulatingAABB()
        {
            // Transform the OBB's eight corner vertices into world space
            for (int i = 0; i < 8; i++)
            {
                this.cornerBuffer[i] = this.orientation * ((this.size * 0.5f) * GetVertexSigns(i)) + this.center;
            }

            // Find the minimum and maximum x, y, and z coordinates
            Vector3 min = this.cornerBuffer[0];
            Vector3 max = this.cornerBuffer[0];
            for (int i = 1; i < 8; i++)
            {
                min.X = Math.Min(min.X, this.cornerBuffer[i].X);
                min.Y = Math.Min(min.Y, this.cornerBuffer[i].Y);
                min.Z = Math.Min(min.Z, this.cornerBuffer[i].Z);
                max.X = Math.Max(max.X, this.cornerBuffer[i].X);
                max.Y = Math.Max(max.Y, this.cornerBuffer[i].Y);
                max.Z = Math.Max(max.Z, this.cornerBuffer[i].Z);
            }

            // Return the AABB as a Bounds object
            return new AABB((min + max) * 0.5f, (max - min) * 0.5f);
        }

        private Vector3 GetVertexSigns(int i)
        {
            return new Vector3(
                (i & 1) == 0 ? 1 : -1,
                (i & 2) == 0 ? 1 : -1,
                (i & 4) == 0 ? 1 : -1
            );
        }
    }
}
