using MathUtils;

namespace ProjectWS.Engine.Mesh
{
    public abstract class Mesh
    {
        const float EPSILON = 0.0000001f;

        public abstract void Build();
        public abstract void Draw();
        public abstract void DrawInstanced();

        public abstract bool MeshIntersectsRay(Ray ray, Vector3 wPos, Quaternion wRot, Vector3 wScale, ref Vector3[] points);

        public bool TriangleIntersectsRay(Ray ray, Vector3 v0, Vector3 v1, Vector3 v2, out Vector3 point)
        {
            point = Vector3.Zero;
            
            Vector3 edge1, edge2, h, s, q;
            float a, f, u, v;
            edge1 = v1 - v0;
            edge2 = v2 - v0;
            h = Vector3.Cross(ray.direction, edge2);
            a = Vector3.Dot(edge1, h);

            if (a > -EPSILON && a < EPSILON)
                return false;    // This ray is parallel to this triangle.

            f = 1.0f / a;
            s = ray.origin - v0;
            u = f * Vector3.Dot(s, h);

            if (u < 0.0f || u > 1.0f)
                return false;

            q = Vector3.Cross(s, edge1);
            v = f * Vector3.Dot(ray.direction, q);

            if (v < 0.0f || u + v > 1.0f)
                return false;

            // At this stage we can compute t to find out where the intersection point is on the line.
            float t = f * Vector3.Dot(edge2, q);

            if (t > EPSILON) // ray intersection
            {
                point = ray.origin + ray.direction * t;
                return true;
            }

            else // This means that there is a line intersection but not a ray intersection.
                return false;
        }

        public Vector3 RotatePointAroundPivot(Vector3 point, Vector3 pivot, Quaternion angles)
        {
            Vector3 dir = point - pivot;        // get point direction relative to pivot
            dir = angles * dir;                 // rotate it
                point = dir + pivot;            // calculate rotated point
            return point;                       // return it
        }
    }
}
