using OpenTK.Graphics.OpenGL4;

namespace ProjectWS.Engine.Data.ResourceManager
{
    public class TextureResource
    {
        // Identifiers //
        string filePath;

        // Reference //
        public Manager manager;

        // Data //
        public ProjectWS.FileFormats.Tex.File? tex;
        public uint texturePtr;
        public Manager.ResourceState state;

        // Info //
        int referenceCount;
        int width;
        int height;
        bool hasMipmaps;
        int mipmapCount;

        // Usage //
        public List<MaterialReference> materialReferences;

        public TextureResource(string filePath, Manager manager)
        {
            this.filePath = filePath;
            this.manager = manager;
            this.state = Manager.ResourceState.IsLoading;
        }

        public void SetResourceState(Manager.ResourceState state)
        {
            this.state = state;
        }

        public void Read()
        {
            this.state = Manager.ResourceState.IsLoading;

            this.tex = new ProjectWS.FileFormats.Tex.File(this.filePath);
            using (var fs = DataManager.GetFileStream(this.filePath))
            {
                if (fs != null)
                    this.tex.Read(fs);
            }

            if (this.tex.failedReading || this.tex.header == null) return;

            this.width = this.tex.header.width;
            this.height = this.tex.header.height;
            this.hasMipmaps = this.tex.header.mipCount > 1;
            this.mipmapCount = this.tex.header.mipCount;
        }

        public bool AssignTexture(Material.Material material, string samplerName)
        {
            this.referenceCount++;

            if (this.state != Manager.ResourceState.IsReady)
            {
                // File Unavailable yet //
                if (this.materialReferences == null) this.materialReferences = new List<MaterialReference>();
                this.materialReferences.Add(new MaterialReference() { material = material, samplerName = samplerName });
                return false;
            }
            else if (this.state == Manager.ResourceState.IsReady)
            {
                //this.texture.wrapMode = wrapMode;
                material.SetTexture(samplerName, this.texturePtr);
                return true;
            }

            return false;
        }

        public void AssignTextureToAllMatRef()
        {
            if (this.materialReferences != null)
            {
                for (int i = 0; i < this.materialReferences.Count; i++)
                {
                    var mRef = this.materialReferences[i];

                    if (this.texturePtr == 0) continue;

                    //this.texture.wrapMode = mRef.wrapMode;

                    if (mRef.material != null)
                    {
                        mRef.material.SetTexture(mRef.samplerName, this.texturePtr);
                    }
                }

                this.materialReferences.Clear();
            }
        }

        public uint LoadTexture()
        {
            GL.GenTextures(1, out this.texturePtr);

            if (this.tex.mipData != null)
            {
                if (this.tex.mipData.Count > 0)
                {
                    GL.BindTexture(TextureTarget.Texture2D, this.texturePtr);
                    GL.TexParameter(TextureTarget.Texture2D, TextureParameterName.TextureWrapS, (int)TextureWrapMode.Repeat);
                    GL.TexParameter(TextureTarget.Texture2D, TextureParameterName.TextureWrapT, (int)TextureWrapMode.Repeat);
                    GL.TexParameter(TextureTarget.Texture2D, TextureParameterName.TextureMinFilter, (int)TextureMinFilter.LinearMipmapLinear);
                    GL.TexParameter(TextureTarget.Texture2D, TextureParameterName.TextureMagFilter, (int)TextureMagFilter.Linear);
                    GL.TexParameter(TextureTarget.Texture2D, TextureParameterName.GenerateMipmap, (int)0);
                    GL.TexParameter(TextureTarget.Texture2D, TextureParameterName.TextureBaseLevel, (int)0);
                    GL.TexParameter(TextureTarget.Texture2D, TextureParameterName.TextureMaxLevel, (int)this.tex.header.mipCount);
                    GL.TexParameter(TextureTarget.Texture2D, TextureParameterName.TextureLodBias, -1);

                    int dataCount = this.tex.mipData.Count;
                    if (this.tex.header.mipCount == 1)
                    {
                        if (this.tex.header.format == 0 || this.tex.header.format == 1)
                            GL.TexImage2D(TextureTarget.Texture2D, 0, PixelInternalFormat.Rgba, this.width, this.height, 0, OpenTK.Graphics.OpenGL4.PixelFormat.Rgba, PixelType.UnsignedByte, this.tex.mipData[0]);
                        else if (this.tex.header.format == 13)
                            GL.CompressedTexImage2D(TextureTarget.Texture2D, 0, InternalFormat.CompressedRgbaS3tcDxt1Ext, this.width, this.height, 0, this.tex.mipData[0].Length, this.tex.mipData[0]);
                        else if (this.tex.header.format == 14)
                            GL.CompressedTexImage2D(TextureTarget.Texture2D, 0, InternalFormat.CompressedRgbaS3tcDxt3Ext, this.width, this.height, 0, this.tex.mipData[0].Length, this.tex.mipData[0]);
                        else if (this.tex.header.format == 15)
                            GL.CompressedTexImage2D(TextureTarget.Texture2D, 0, InternalFormat.CompressedRgbaS3tcDxt5Ext, this.width, this.height, 0, this.tex.mipData[0].Length, this.tex.mipData[0]);
                        // TODO : Missing Rgb and Grayscale
                    }
                    else
                    {
                        for (int i = 0; i < this.tex.header.mipCount; i++)
                        {
                            int dataIdx = this.tex.mipData.Count - i - 1;
                            if (this.tex.header.format == 0)
                                dataIdx = Math.Min(dataCount - 1, (int)(this.tex.header.imageSizesCount - i) - 1);

                            int w = Math.Max(1, (int)(this.width / Math.Pow(2, i)));
                            int h = Math.Max(1, (int)(this.height / Math.Pow(2, i)));
                            if (dataIdx >= 0)
                            {
                                if (this.tex.header.format == 0 || this.tex.header.format == 1)
                                    GL.TexImage2D(TextureTarget.Texture2D, i, PixelInternalFormat.Rgba, w, h, 0, OpenTK.Graphics.OpenGL4.PixelFormat.Rgba, PixelType.UnsignedByte, this.tex.mipData[dataIdx]);
                                else if (this.tex.header.format == 13)
                                    GL.CompressedTexImage2D(TextureTarget.Texture2D, i, InternalFormat.CompressedRgbaS3tcDxt1Ext, w, h, 0, this.tex.mipData[dataIdx].Length, this.tex.mipData[dataIdx]);
                                else if (this.tex.header.format == 14)
                                    GL.CompressedTexImage2D(TextureTarget.Texture2D, i, InternalFormat.CompressedRgbaS3tcDxt3Ext, w, h, 0, this.tex.mipData[dataIdx].Length, this.tex.mipData[dataIdx]);
                                else if (this.tex.header.format == 15)
                                    GL.CompressedTexImage2D(TextureTarget.Texture2D, i, InternalFormat.CompressedRgbaS3tcDxt5Ext, w, h, 0, this.tex.mipData[dataIdx].Length, this.tex.mipData[dataIdx]);
                                // TODO : Missing Rgb and Grayscale
                            }
                        }
                    }
                }
                //GL.GenerateMipmap(GenerateMipmapTarget.Texture2D);
            }

            this.state = Manager.ResourceState.IsReady;

            return this.texturePtr;
        }

        public void RemoveTexture()
        {
            referenceCount--;

            if (referenceCount == 0)
            {
                if (this.texturePtr != 0)
                {
                    GL.DeleteTexture(this.texturePtr);
                }

                this.state = Manager.ResourceState.None;
                this.tex = null;
                this.manager.textureResources.Remove(this.filePath);
            }
        }

        public struct MaterialReference
        {
            public Material.Material material;
            public string samplerName;
            //public TextureWrapMode wrapMode;
        }
    }
}