using OpenTK.Graphics.OpenGL4;
using MathUtils;
using ProjectWS.Engine.Rendering.ShaderParams;
using ProjectWS.Engine.Data;
using ProjectWS.Engine.Data.ResourceManager;

namespace ProjectWS.Engine.Material
{
    public class TerrainMaterial : Material
    {
        FileFormats.Area.SubArea subChunkData;
        uint blendMapPtr;
        uint colorMapPtr;
        uint unkMap2Ptr;
        Vector4 heightScale = Vector4.One;
        Vector4 heightOffset = Vector4.Zero;
        Vector4 parallaxScale = Vector4.One;
        Vector4 parallaxOffset = Vector4.Zero;
        Vector4 metersPerTextureTile;

        const string LAYER0 = "layer0";
        const string LAYER1 = "layer1";
        const string LAYER2 = "layer2";
        const string LAYER3 = "layer3";
        const string NORMAL0 = "normal0";
        const string NORMAL1 = "normal1";
        const string NORMAL2 = "normal2";
        const string NORMAL3 = "normal3";

        const string GREY_DEFAULT = "Art\\Dev\\BLANK_Grey.tex";
        const string NORMAL_DEFAULT = "Art\\Dev\\BLANK_Normal.tex";

        Dictionary<string, int> textureResources = new Dictionary<string, int>();

        TerrainParameters tParams;

        public TerrainMaterial(FileFormats.Area.SubArea subChunkData)
        {
            this.tParams = new TerrainParameters();
            this.subChunkData = subChunkData;
        }

        public override void Build()
        {
            if (this.isBuilt || this.isBuilding) return;

            this.isBuilding = true;

            this.texturePtrs = new Dictionary<string, uint>();

            if (this.subChunkData.blendMapMode == FileFormats.Area.SubArea.MapMode.DXT1)
                BuildMap(this.subChunkData.blendMap, InternalFormat.CompressedRgbaS3tcDxt1Ext, out blendMapPtr);
            else if (this.subChunkData.blendMapMode == FileFormats.Area.SubArea.MapMode.Raw)
                BuildMap(this.subChunkData.blendMap, InternalFormat.Rgba, out blendMapPtr);

            BuildMap(this.subChunkData.unknownMap2, InternalFormat.CompressedRgbaS3tcDxt1Ext, out unkMap2Ptr);

            if (this.subChunkData.colorMapMode == FileFormats.Area.SubArea.MapMode.DXT5)
                BuildMap(this.subChunkData.colorMap, InternalFormat.CompressedRgbaS3tcDxt5Ext, out colorMapPtr);
            else if (this.subChunkData.colorMapMode == FileFormats.Area.SubArea.MapMode.Raw)
                BuildMap(this.subChunkData.colorMap, InternalFormat.Rgba, out colorMapPtr);

            if (this.subChunkData.worldLayerIDs != null)
            {
                for (int i = 0; i < 4; i++)
                {
                    if (this.subChunkData.worldLayerIDs[i] != 0)
                    {
                        var record = DataManager.database.worldLayer.Get(this.subChunkData.worldLayerIDs[i]);
                        if (record != null)
                        {
                            DataManager.engine.resourceManager.AssignTexture(record.ColorMapPath, this, $"layer{i}");
                            DataManager.engine.resourceManager.AssignTexture(record.NormalMapPath, this, $"normal{i}");

                            if (textureResources.ContainsKey(record.ColorMapPath))
                                textureResources[record.ColorMapPath]++;
                            else
                                textureResources.Add(record.ColorMapPath, 1);

                            if (textureResources.ContainsKey(record.NormalMapPath))
                                textureResources[record.NormalMapPath]++;
                            else
                                textureResources.Add(record.NormalMapPath, 1);

                            heightScale[i] = record.HeightScale;
                            heightOffset[i] = record.HeightOffset;
                            parallaxScale[i] = record.ParallaxScale;
                            parallaxOffset[i] = record.ParallaxOffset;
                            metersPerTextureTile[i] = record.MetersPerTextureTile;
                        }
                    }
                }
            }
            else
            {

                DataManager.engine?.resourceManager?.AssignTexture(GREY_DEFAULT, this, LAYER0);
                DataManager.engine?.resourceManager?.AssignTexture(NORMAL_DEFAULT, this, NORMAL0);

                if (textureResources.ContainsKey(GREY_DEFAULT))
                    textureResources[GREY_DEFAULT]++;
                else
                    textureResources.Add(GREY_DEFAULT, 1);

                if (textureResources.ContainsKey(NORMAL_DEFAULT))
                    textureResources[NORMAL_DEFAULT]++;
                else
                    textureResources.Add(NORMAL_DEFAULT, 1);

                for (int i = 0; i < 4; i++)
                {
                    metersPerTextureTile[i] = 1.0f;
                }
            }

            this.isBuilt = true;
        }

        public override void SetToShader(Shader shader)
        {
            if (!this.isBuilt) return;

            if (this.texturePtrs.TryGetValue(LAYER0, out uint layer0Ptr))
            {
                GL.ActiveTexture(TextureUnit.Texture0);
                GL.BindTexture(TextureTarget.Texture2D, layer0Ptr);
            }
            else
            {
                GL.ActiveTexture(TextureUnit.Texture0);
                GL.BindTexture(TextureTarget.Texture2D, -1);
            }

            if (this.texturePtrs.TryGetValue(LAYER1, out uint layer1Ptr))
            {
                GL.ActiveTexture(TextureUnit.Texture1);
                GL.BindTexture(TextureTarget.Texture2D, layer1Ptr);
            }
            else
            {
                GL.ActiveTexture(TextureUnit.Texture1);
                GL.BindTexture(TextureTarget.Texture2D, -1);
            }

            if (this.texturePtrs.TryGetValue(LAYER2, out uint layer2Ptr))
            {
                GL.ActiveTexture(TextureUnit.Texture2);
                GL.BindTexture(TextureTarget.Texture2D, layer2Ptr);
            }
            else
            {
                GL.ActiveTexture(TextureUnit.Texture2);
                GL.BindTexture(TextureTarget.Texture2D, -1);
            }

            if (this.texturePtrs.TryGetValue(LAYER3, out uint layer3Ptr))
            {
                GL.ActiveTexture(TextureUnit.Texture3);
                GL.BindTexture(TextureTarget.Texture2D, layer3Ptr);
            }
            else
            {
                GL.ActiveTexture(TextureUnit.Texture3);
                GL.BindTexture(TextureTarget.Texture2D, -1);
            }

            if (this.texturePtrs.TryGetValue(NORMAL0, out uint normal0Ptr))
            {
                GL.ActiveTexture(TextureUnit.Texture4);
                GL.BindTexture(TextureTarget.Texture2D, normal0Ptr);
            }

            if (this.texturePtrs.TryGetValue(NORMAL1, out uint normal1Ptr))
            {
                GL.ActiveTexture(TextureUnit.Texture5);
                GL.BindTexture(TextureTarget.Texture2D, normal1Ptr);
            }

            if (this.texturePtrs.TryGetValue(NORMAL2, out uint normal2Ptr))
            {
                GL.ActiveTexture(TextureUnit.Texture6);
                GL.BindTexture(TextureTarget.Texture2D, normal2Ptr);
            }

            if (this.texturePtrs.TryGetValue(NORMAL3, out uint normal3Ptr))
            {
                GL.ActiveTexture(TextureUnit.Texture7);
                GL.BindTexture(TextureTarget.Texture2D, normal3Ptr);
            }

            GL.ActiveTexture(TextureUnit.Texture8);
            GL.BindTexture(TextureTarget.Texture2D, this.blendMapPtr);

            this.tParams.enableColorMap = this.colorMapPtr != 0;

            if (this.tParams.enableColorMap)
            {
                GL.ActiveTexture(TextureUnit.Texture9);
                GL.BindTexture(TextureTarget.Texture2D, this.colorMapPtr);
            }

            if (this.unkMap2Ptr != 0)
            {
                GL.ActiveTexture(TextureUnit.Texture10);
                GL.BindTexture(TextureTarget.Texture2D, this.unkMap2Ptr);
            }

            this.tParams.heightScale = this.heightScale;
            this.tParams.heightOffset = this.heightOffset;
            this.tParams.parallaxScale = this.parallaxScale;
            this.tParams.parallaxOffset = this.parallaxOffset;
            this.tParams.metersPerTextureTile = this.metersPerTextureTile;

            this.tParams.SetToShader(shader);
        }

        void BuildMap(byte[] data, InternalFormat format, out uint ptr)
        {
            if (data == null) { ptr = 0; return; }

            GL.GenTextures(1, out ptr);
            GL.BindTexture(TextureTarget.Texture2D, ptr);
            GL.TexParameter(TextureTarget.Texture2D, TextureParameterName.TextureWrapS, (int)TextureWrapMode.ClampToEdge);
            GL.TexParameter(TextureTarget.Texture2D, TextureParameterName.TextureWrapT, (int)TextureWrapMode.ClampToEdge);
            GL.TexParameter(TextureTarget.Texture2D, TextureParameterName.TextureMinFilter, (int)TextureMinFilter.LinearMipmapLinear);
            GL.TexParameter(TextureTarget.Texture2D, TextureParameterName.TextureMagFilter, (int)TextureMagFilter.Linear);
            GL.TexParameter(TextureTarget.Texture2D, TextureParameterName.GenerateMipmap, 0);
            GL.TexParameter(TextureTarget.Texture2D, TextureParameterName.TextureBaseLevel, 0);
            GL.TexParameter(TextureTarget.Texture2D, TextureParameterName.TextureMaxLevel, 0);
            
            if (format == InternalFormat.Rgba)
            {
                GL.TexImage2D(TextureTarget.Texture2D, 0, PixelInternalFormat.Rgba, 65, 65, 0, PixelFormat.Rgba, PixelType.UnsignedByte, data);
            }
            else
            {
                GL.CompressedTexImage2D(TextureTarget.Texture2D, 0, format, 65, 65, 0, data.Length, data);
            }
        }

        public void UpdateBlendMap(byte[] data)
        {
            GL.BindTexture(TextureTarget.Texture2D, this.blendMapPtr);
            GL.TexImage2D(TextureTarget.Texture2D, 0, PixelInternalFormat.Rgba, 65, 65, 0, PixelFormat.Rgba, PixelType.UnsignedByte, data);
        }

        internal void Unload()
        {
            if (this.blendMapPtr != 0)
                GL.DeleteTexture(this.blendMapPtr);

            if (this.colorMapPtr != 0)
                GL.DeleteTexture(this.colorMapPtr);

            if (this.unkMap2Ptr != 0)
                GL.DeleteTexture(this.unkMap2Ptr);

            foreach (var item in textureResources)
            {
                for (int i = 0; i < item.Value; i++)
                {
                    DataManager.engine?.resourceManager?.RemoveTexture(item.Key);
                }
            }
        }
    }
}
