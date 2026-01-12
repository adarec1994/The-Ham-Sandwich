using OpenTK;
using OpenTK.Graphics.OpenGL4;
using MathUtils;
using ProjectWS.Engine.Rendering.ShaderParams;
using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace ProjectWS.Engine.Material
{
    public class WaterMaterial : Material
    {
        FileFormats.Area.Water water;
        //WaterParameters wParams;

        public WaterMaterial(FileFormats.Area.Water water)
        {
            this.water = water;
            //this.wParams = new WaterParameters();
        }

        public override void Build()
        {
            if (this.isBuilt || this.isBuilding) return;

            this.isBuilding = true;

            this.texturePtrs = new Dictionary<string, uint>();
            /*
            BuildMap(this.water.blendMap, InternalFormat.CompressedRgbaS3tcDxt1Ext, out blendMapPtr);
            BuildMap(this.water.colorMap, InternalFormat.CompressedRgbaS3tcDxt5Ext, out colorMapPtr);

            if (this.water.textureIDs != null)
            {
                for (int i = 0; i < 4; i++)
                {
                    if (this.water.textureIDs[i] != 0)
                    {
                        var record = this.water.chunk.gameData.database.worldLayer.Get(this.water.textureIDs[i]);
                        if (record != null)
                        {
                            this.water.chunk.gameData.resourceManager.AssignTexture(record.ColorMapPath, this, $"layer{i}");
                            this.water.chunk.gameData.resourceManager.AssignTexture(record.NormalMapPath, this, $"normal{i}");

                            heightScale[i] = record.HeightScale;
                            heightOffset[i] = record.HeightOffset;
                            parallaxScale[i] = record.ParallaxScale;
                            parallaxOffset[i] = record.ParallaxOffset;
                            metersPerTextureTile[i] = record.MetersPerTextureTile;
                        }
                    }
                }
            }

            this.isBuilt = true;
            */
        }

        public override void SetToShader(Shader shader)
        {
            if (!this.isBuilt) return;
            /*
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

            this.tParams.heightScale = this.heightScale;
            this.tParams.heightOffset = this.heightOffset;
            this.tParams.parallaxScale = this.parallaxScale;
            this.tParams.parallaxOffset = this.parallaxOffset;
            this.tParams.metersPerTextureTile = this.metersPerTextureTile;

            this.tParams.SetToShader(shader);
            */
        }

        internal void Unload()
        {
            
        }

        void BuildMap(byte[] data, InternalFormat format, out uint ptr)
        {
            if (data == null) { ptr = 0; return; }

            GL.GenTextures(1, out ptr);
            GL.BindTexture(TextureTarget.Texture2D, ptr);
            GL.TexParameter(TextureTarget.Texture2D, TextureParameterName.TextureWrapS, (int)TextureWrapMode.ClampToEdge);
            GL.TexParameter(TextureTarget.Texture2D, TextureParameterName.TextureWrapT, (int)TextureWrapMode.ClampToEdge);
            GL.TexParameter(TextureTarget.Texture2D, TextureParameterName.TextureMinFilter, (int)TextureMinFilter.Linear);
            GL.TexParameter(TextureTarget.Texture2D, TextureParameterName.TextureMagFilter, (int)TextureMagFilter.Linear);
            GL.TexParameter(TextureTarget.Texture2D, TextureParameterName.GenerateMipmap, 0);
            GL.TexParameter(TextureTarget.Texture2D, TextureParameterName.TextureBaseLevel, 0);
            GL.TexParameter(TextureTarget.Texture2D, TextureParameterName.TextureMaxLevel, 0);
            GL.CompressedTexImage2D(TextureTarget.Texture2D, 0, format, 65, 65, 0, data.Length, data);
        }
    }
}
