using MathUtils;

namespace ProjectWS.Engine.Lighting
{
    public abstract class Light : Objects.GameObject
    {
        public Vector4 color;

        public Light()
        {

        }

        public abstract void ApplyToShader(Shader shader);
    }
}
