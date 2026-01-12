using OpenTK.Graphics.OpenGL4;
using MathUtils;

namespace ProjectWS.Engine.Rendering.ShaderParams
{
    public class TerrainParameters
    {
        public Vector4 heightScale;
        public Vector4 heightOffset;
        public Vector4 parallaxScale;
        public Vector4 parallaxOffset;
        public Vector4 metersPerTextureTile;
        public float specularPower;
        public Vector2 scrollSpeed;
        public bool enableColorMap;
        public bool enableUnkMap2;

        public TerrainParameters()
        {
            this.metersPerTextureTile = new Vector4(32.0f, 32.0f, 32.0f, 32.0f);
        }


        public void SetToShader(Shader shader)
        {
            GL.Uniform4(GL.GetUniformLocation(shader.Handle, "terrainParams.heightScale"), this.heightScale.X, this.heightScale.Y, this.heightScale.Z, this.heightScale.W);
            GL.Uniform4(GL.GetUniformLocation(shader.Handle, "terrainParams.heightOffset"), this.heightOffset.X, this.heightOffset.Y, this.heightOffset.Z, this.heightOffset.W);
            GL.Uniform4(GL.GetUniformLocation(shader.Handle, "terrainParams.parallaxScale"), this.parallaxScale.X, this.parallaxScale.Y, this.parallaxScale.Z, this.parallaxScale.W);
            GL.Uniform4(GL.GetUniformLocation(shader.Handle, "terrainParams.parallaxOffset"), this.parallaxOffset.X, this.parallaxOffset.Y, this.parallaxOffset.Z, this.parallaxOffset.W);
            GL.Uniform4(GL.GetUniformLocation(shader.Handle, "terrainParams.metersPerTextureTile"), this.metersPerTextureTile.X, this.metersPerTextureTile.Y, this.metersPerTextureTile.Z, this.metersPerTextureTile.W);
            GL.Uniform1(GL.GetUniformLocation(shader.Handle, "terrainParams.specularPower"), this.specularPower);
            GL.Uniform2(GL.GetUniformLocation(shader.Handle, "terrainParams.scrollSpeed"), this.scrollSpeed.X, this.scrollSpeed.Y);
            GL.Uniform1(GL.GetUniformLocation(shader.Handle, "terrainParams.enableColorMap"), this.enableColorMap ? 1 : 0);
            GL.Uniform1(GL.GetUniformLocation(shader.Handle, "terrainParams.enableUnkMap2"), this.enableUnkMap2 ? 1 : 0);
        }
    }
}