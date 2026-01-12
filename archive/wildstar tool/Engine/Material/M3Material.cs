using OpenTK.Graphics.OpenGL4;
using ProjectWS.Engine.Data;
using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace ProjectWS.Engine.Material
{
    public class M3Material : Material
    {
        FileFormats.M3.Material matData;
        FileFormats.M3.File m3;
        ProjectWS.Engine.Data.ResourceManager.Manager rm;
        Dictionary<string, int> textureResources = new Dictionary<string, int>();

        public M3Material(FileFormats.M3.Material matData, FileFormats.M3.File m3, ProjectWS.Engine.Data.ResourceManager.Manager? rm)
        {
            this.matData = matData;
            this.m3 = m3;
            this.rm = rm;
        }

        public override void Build()
        {
            if (this.isBuilt || this.isBuilding) return;

            this.isBuilding = true;

            this.texturePtrs = new Dictionary<string, uint>();

            for (int i = 0; i < this.matData.materialDescriptions.Length; i++)
            {
                short textureA = this.matData.materialDescriptions[i].textureSelectorA;
                if (textureA != -1)
                {
                    var texture = m3.textures[textureA];

                    if (texture.textureType == FileFormats.M3.Texture.TextureType.Diffuse)
                        rm.AssignTexture(texture.texturePath, this, $"diffuseMap{i}");
                    else if (texture.textureType == FileFormats.M3.Texture.TextureType.Normal)
                        rm.AssignTexture(texture.texturePath, this, $"normalMap{i}");

                    if (textureResources.ContainsKey(texture.texturePath))
                        textureResources[texture.texturePath]++;
                    else
                        textureResources.Add(texture.texturePath, 1);
                }

                short textureB = this.matData.materialDescriptions[i].textureSelectorB;
                if (textureB != -1)
                {
                    var texture = m3.textures[textureB];
                    if (texture.textureType == FileFormats.M3.Texture.TextureType.Diffuse)
                        rm.AssignTexture(texture.texturePath, this, $"diffuseMap{i}");
                    else if (texture.textureType == FileFormats.M3.Texture.TextureType.Normal)
                        rm.AssignTexture(texture.texturePath, this, $"normalMap{i}");

                    if (textureResources.ContainsKey(texture.texturePath))
                        textureResources[texture.texturePath]++;
                    else
                        textureResources.Add(texture.texturePath, 1);
                }
            }


            this.isBuilt = true;
        }

        public override void SetToShader(Shader shader)
        {
            if (!this.isBuilt) return;

            for (int j = 0; j < 4; j++)
            {
                // Diffuse Maps //
                if (j == 0)
                    GL.ActiveTexture(TextureUnit.Texture0);
                else if (j == 1)
                    GL.ActiveTexture(TextureUnit.Texture2);
                else if (j == 2)
                    GL.ActiveTexture(TextureUnit.Texture4);
                else if (j == 3)
                    GL.ActiveTexture(TextureUnit.Texture6);

                if (this.texturePtrs.TryGetValue($"diffuseMap{j}", out uint texDiffusePtr))
                {
                    GL.BindTexture(TextureTarget.Texture2D, texDiffusePtr);
                }
                else
                {
                    GL.BindTexture(TextureTarget.Texture2D, -1);
                }

                // Normal Maps //
                if (j == 0)
                    GL.ActiveTexture(TextureUnit.Texture1);
                else if (j == 1)
                    GL.ActiveTexture(TextureUnit.Texture3);
                else if (j == 2)
                    GL.ActiveTexture(TextureUnit.Texture5);
                else if (j == 3)
                    GL.ActiveTexture(TextureUnit.Texture7);

                if (this.texturePtrs.TryGetValue($"normalMap{j}", out uint texNormalPtr))
                {
                    GL.BindTexture(TextureTarget.Texture2D, texNormalPtr);
                }
                else
                {
                    GL.BindTexture(TextureTarget.Texture2D, -1);
                }
            }

        }

        internal void Unload()
        {
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
