using OpenTK.Graphics.OpenGL4;
using MathUtils;

namespace ProjectWS.Engine.Rendering.ShaderParams
{
    public class EnvironmentParameters
    {
        public Vector3 ambientColor;

        public bool isEnabled;

        public EnvironmentParameters(Vector3 ambientColor)
        {
            this.ambientColor = ambientColor;
            this.isEnabled = true;
        }

        public EnvironmentParameters(Color ambientColor)
        {
            this.ambientColor = new Vector3(ambientColor.R, ambientColor.G, ambientColor.B);
            this.isEnabled = true;
        }

        public void SetToShader(Shader shader)
        {
            GL.Uniform3(GL.GetUniformLocation(shader.Handle, "envParams.ambientColor"), this.ambientColor.X, this.ambientColor.Y, this.ambientColor.Z);
            GL.Uniform1(GL.GetUniformLocation(shader.Handle, "envParams.isEnabled"), this.isEnabled ? 1 : 0);
        }

        public void Toggle(bool on)
        {
            this.isEnabled = on;
        }
    }
}