using MathUtils;

namespace ProjectWS.Engine.Lighting
{
    public class DirectLight : Light
    {
        public DirectLight(Vector3 position, Vector4 color) : base()
        {
            this.transform.SetPosition(position);
            this.color = color;
        }

        public override void ApplyToShader(Shader shader)
        {
            shader.SetColor4("lightColor", this.color);
            shader.SetVec3("lightPos", this.transform.GetPosition());
        }
    }
}
