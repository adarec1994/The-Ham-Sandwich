using OpenTK.Graphics.OpenGL4;
using MathUtils;

namespace ProjectWS.Engine.Rendering.ShaderParams
{
    public class BrushParameters
    {
        public BrushMode mode;
        public Vector3 position;
        public float size;

        public bool isEnabled;

        public enum BrushMode
        {
            Gradient = 0,
            Circle = 1,
        }

        public BrushParameters(BrushMode mode, Vector3 position, float size)
        {
            this.mode = mode;
            this.position = position;
            this.size = size;

            this.isEnabled = false;
        }

        public void SetToShader(Shader shader)
        {
            GL.Uniform1(GL.GetUniformLocation(shader.Handle, "tEditorParams.brushParams.mode"), (int)this.mode);
            GL.Uniform3(GL.GetUniformLocation(shader.Handle, "tEditorParams.brushParams.position"), this.position.X, this.position.Y, this.position.Z);
            GL.Uniform1(GL.GetUniformLocation(shader.Handle, "tEditorParams.brushParams.size"), this.size);
            GL.Uniform1(GL.GetUniformLocation(shader.Handle, "tEditorParams.brushParams.isEnabled"), this.isEnabled ? 1 : 0);
        }

        public void Toggle(bool on)
        {
            this.isEnabled = on;
        }
    }
}