using MathUtils;
using OpenTK.Graphics.OpenGL4;

namespace ProjectWS.Engine.World
{
    public class MinimapChunk
    {
        public bool exists;
        public string? path;
        public FileFormats.Tex.File? texFile;
        internal bool isVisible;
        internal volatile bool isRead;
        public Matrix4 matrix;

        int[] minimapPtr;
        int mipCount;

        public void Read()
        {
            if (this.path == null || this.texFile == null)
            {
                this.exists = false;
                this.isRead = false;
                return;
            }

            using (var fs = File.OpenRead(this.path))
            {
                this.texFile.Read(fs);

                if (this.texFile.mipData == null)
                {
                    this.exists = false;
                    this.isRead = false;
                    return;
                }

                this.mipCount = this.texFile.mipData.Count;
                this.minimapPtr = new int[this.mipCount];
            }

            this.isRead = true;
        }

        public void BuildMip(int mip)
        {
            var resolution = 512;
            for (int i = 0; i < mip; i++)
            {
                resolution /= 2;
            }
            var mipIndex = (this.mipCount - 1) - mip;

            if (this.texFile == null)
                return;

            if (this.texFile.mipData == null)
                return;

            if (mipIndex >= this.texFile.mipData.Count || mipIndex < 0)
                return;

            GL.GenTextures(1, out this.minimapPtr[mipIndex]);
            GL.BindTexture(TextureTarget.Texture2D, this.minimapPtr[mipIndex]);
            GL.TexParameter(TextureTarget.Texture2D, TextureParameterName.TextureWrapS, (int)TextureWrapMode.ClampToEdge);
            GL.TexParameter(TextureTarget.Texture2D, TextureParameterName.TextureWrapT, (int)TextureWrapMode.ClampToEdge);
            GL.TexParameter(TextureTarget.Texture2D, TextureParameterName.TextureMinFilter, (int)TextureMinFilter.LinearMipmapLinear);
            GL.TexParameter(TextureTarget.Texture2D, TextureParameterName.TextureMagFilter, (int)TextureMagFilter.Linear);
            GL.TexParameter(TextureTarget.Texture2D, TextureParameterName.GenerateMipmap, 0);
            GL.TexParameter(TextureTarget.Texture2D, TextureParameterName.TextureBaseLevel, 0);
            GL.TexParameter(TextureTarget.Texture2D, TextureParameterName.TextureMaxLevel, 0);

            try
            {
                var data = this.texFile.mipData[mipIndex];
                GL.CompressedTexImage2D(TextureTarget.Texture2D, 0, InternalFormat.CompressedRgbaS3tcDxt1Ext, resolution, resolution, 0, data.Length, data);
            }
            catch{ }
        }

        internal void Render(Shader shader, int mip, int quadVAO)
        {
            var mipIndex = (this.mipCount - 1) - mip;

            if (this.minimapPtr == null)
                this.minimapPtr = new int[this.mipCount];

            if (mipIndex == -1 || mipIndex >= this.mipCount)
                mipIndex = this.mipCount - 1;

            if (this.minimapPtr[mipIndex] == 0)
                BuildMip(mip);

            GL.ActiveTexture(TextureUnit.Texture0);
            GL.BindTexture(TextureTarget.Texture2D, this.minimapPtr[mipIndex]);

            shader.SetMat4("model", ref matrix);

            GL.BindVertexArray(quadVAO);
            GL.DrawArrays(PrimitiveType.TriangleStrip, 0, 4);
        }

        public void Clear()
        {
            if (this.exists)
            {
                this.path = null;
                this.texFile = null;
                this.isVisible = false;
                this.isRead = false;
                if (this.minimapPtr != null)
                {
                    for (int i = 0; i < this.minimapPtr.Length; i++)
                    {
                        if (this.minimapPtr[i] != 0)
                            GL.DeleteTexture(this.minimapPtr[i]);
                    }
                }
                this.mipCount = 0;
            }
        }
    }
}
