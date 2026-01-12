using OpenTK.Graphics.OpenGL4;
using MathUtils;

namespace ProjectWS.Engine.Rendering.ShaderParams
{
    public class SunParameters
    {
        public Vector3 color;
        public Vector3 direction;
        public float intensity;

        public bool isEnabled;

        public SunParameters(Vector3 color, Vector3 direction, float intensity)
        {
            this.color = color;
            this.direction = direction;
            this.intensity = intensity;
            this.isEnabled = true;
        }

        public SunParameters(Color color, Vector3 direction, float intensity)
        {
            this.color = new Vector3(color.R, color.G, color.B);
            this.direction = direction;
            this.intensity = intensity;
            this.isEnabled = true;
        }

        public void SetToShader(Shader shader)
        {
            GL.Uniform3(GL.GetUniformLocation(shader.Handle, "envParams.sunParams.color"), this.color.X, this.color.Y, this.color.Z);
            GL.Uniform3(GL.GetUniformLocation(shader.Handle, "envParams.sunParams.direction"), this.direction.X, this.direction.Y, this.direction.Z);
            GL.Uniform1(GL.GetUniformLocation(shader.Handle, "envParams.sunParams.intensity"), this.intensity);
            GL.Uniform1(GL.GetUniformLocation(shader.Handle, "envParams.sunParams.isEnabled"), this.isEnabled ? 1 : 0);
        }

        public void Toggle(bool on)
        {
            this.isEnabled = on;
        }
    }
}