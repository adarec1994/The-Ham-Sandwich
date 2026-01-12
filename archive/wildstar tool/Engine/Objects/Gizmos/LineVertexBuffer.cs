using MathUtils;

namespace ProjectWS.Engine.Objects.Gizmos
{
    public struct LineVertexBuffer
    {
        public Vector3 position;
        public Vector4 color;

        public LineVertexBuffer(Vector3 position, Vector4 color)
        {
            this.position = position;
            this.color = color;
        }
    }
}
