using OpenTK.Graphics.OpenGL4;
using MathUtils;
using System.Reflection.Metadata;

namespace ProjectWS.Engine.Rendering.ShaderParams
{
    public class FogParameters
    {
        public Vector3 color;
        public float linearStart;
        public float linearEnd;
        public float density;

        public int equation;
        public bool isEnabled;

        public FogParameters(Vector3 color, float linearStart, float linearEnd, float density, int equation)
        {
            this.color = color;
            this.linearStart = linearStart;
            this.linearEnd = linearEnd;
            this.density = density;
            this.equation = equation;
            isEnabled = true;
        }

        public FogParameters(Color color, float linearStart, float linearEnd, float density, int equation)
        {
            this.color = new Vector3(color.R, color.G, color.B);
            this.linearStart = linearStart;
            this.linearEnd = linearEnd;
            this.density = density;
            this.equation = equation;
            isEnabled = true;
        }

        public void SetToShader(Shader shader)
        {
            GL.Uniform3(GL.GetUniformLocation(shader.Handle, "envParams.fogParams.color"), this.color.X, this.color.Y, this.color.Z);
            GL.Uniform1(GL.GetUniformLocation(shader.Handle, "envParams.fogParams.linearStart"), this.linearStart);
            GL.Uniform1(GL.GetUniformLocation(shader.Handle, "envParams.fogParams.linearEnd"), this.linearEnd);
            GL.Uniform1(GL.GetUniformLocation(shader.Handle, "envParams.fogParams.density"), this.density);
            GL.Uniform1(GL.GetUniformLocation(shader.Handle, "envParams.fogParams.equation"), this.equation);
            GL.Uniform1(GL.GetUniformLocation(shader.Handle, "envParams.fogParams.isEnabled"), this.isEnabled ? 1 : 0);
        }

        public void Toggle(bool on)
        {
            isEnabled = on;
        }
    }
}