using OpenTK.Graphics.OpenGL4;
using MathUtils;

namespace ProjectWS.Engine.Rendering.ShaderParams
{
    public class TerrainEditorParameters
    {
        public bool enableAreaGrid;
        public bool enableChunkGrid;

        public TerrainEditorParameters(bool areaGrid, bool chunkGrid)
        {
            this.enableAreaGrid = areaGrid;
            this.enableChunkGrid = chunkGrid;
        }

        public void SetToShader(Shader shader)
        {
            GL.Uniform1(GL.GetUniformLocation(shader.Handle, "tEditorParams.enableAreaGrid"), this.enableAreaGrid ? 1 : 0);
            GL.Uniform1(GL.GetUniformLocation(shader.Handle, "tEditorParams.enableChunkGrid"), this.enableChunkGrid ? 1 : 0);
        }
    }
}