using ProjectWS.Engine.Rendering;
using System;
using MathUtils;

namespace ProjectWS.Editor.Tools
{
    public class TerrainLayerPaintTool : Tool
    {
        readonly Engine.Engine engine;
        public readonly WorldRenderer? worldRenderer;
        readonly Editor editor;

        public int layer = 1;

        public TerrainLayerPaintTool(Engine.Engine engine, Editor editor, WorldRenderer world)
        {
            this.hasBrush = true;
            this.editor = editor;
            this.engine = engine;
            this.worldRenderer = world;
        }

        public override void Enable()
        {
            this.isEnabled = true;
            if (this.worldRenderer != null)
            {
                if (this.worldRenderer.brushParameters != null)
                    this.worldRenderer.brushParameters.mode = Engine.Rendering.ShaderParams.BrushParameters.BrushMode.Circle;
                if (this.worldRenderer.mousePick != null)
                    this.worldRenderer.mousePick.mode = Engine.MousePick.Mode.Terrain;
            }
        }

        public override void Disable()
        {
            this.isEnabled = false;
            if (this.worldRenderer != null && this.worldRenderer.mousePick != null)
                this.worldRenderer.mousePick.mode = Engine.MousePick.Mode.Disabled;
        }

        public override void Update(float deltaTime)
        {
            if (this.worldRenderer == null) return;
            if (this.worldRenderer.world == null)  return;
            if (this.worldRenderer.mousePick == null) return;

            // Update brush size
            if (this.worldRenderer.brushParameters != null)
            {
                this.worldRenderer.brushParameters.size += this.engine.input.GetMouseScroll();
                this.worldRenderer.brushParameters.size = (float)Math.Clamp(this.worldRenderer.brushParameters.size, 1.0f, 100f);
            }

            float[] perc = new float[4];

            if (this.engine.input.LMB && this.editor.keyboardFocused && this.worldRenderer.brushParameters != null && this.worldRenderer.brushParameters.isEnabled)
            {
                var brushSize = this.worldRenderer.brushParameters.size;
                var hitPoint = this.worldRenderer.mousePick.terrainHitPoint;
                Vector2i terrainAreaHit = this.worldRenderer.mousePick.areaHit;

                for (int a = 0; a < this.areaKernel.Length; a++)
                {
                    if (this.worldRenderer.world.chunks.TryGetValue(terrainAreaHit + this.areaKernel[a], out var chunk))
                    {
                        // TODO : currently just checking all 9 kernels, could optimize this by verifying on which kernel the brush is on
                        // TODO : future, add support for more than 3x3 areas, brush size is now stuck to max 512x512 (although it's probably enough)
                        //var cDist = Vector2.Distance(chunk.worldCoords.Xz + new Vector2(256, 256), hitPoint.Xz);
                        //if (cDist > brushSize * 2 + 256)
                        //    continue;

                        var areaPos = chunk.worldCoords.Xz;

                        for (int s = 0; s < chunk.subChunks.Count; s++)
                        {
                            var scDist = Vector2.Distance(chunk.subChunks[s].centerPosition.Xz, hitPoint.Xz);
                            //float scDist = Vector2.ManhattanDistance(chunk.subChunks[s].centerPosition.Xz, hitPoint.Xz);

                            if (scDist > brushSize + 32f)
                                continue;

                            var sc = chunk.subChunks[s];
                            var subPos = new Vector2(sc.X * 32f, sc.Y * 32f) + areaPos;

                            if (sc.X < 0 || sc.X > 15 || sc.Y < 0 || sc.Y > 15)
                                continue;

                            for (int x = 0; x < 65; x++)
                            {
                                for (int y = 0; y < 65; y++)
                                {
                                    float dist = Vector2.Distance(hitPoint.Xz - subPos, new Vector2(x, y) / 65f * 32f);
                                    //float dist = Vector2.ManhattanDistance(hitPoint.Xz - subPos, new Vector2(x, y) / 65f * 32f);

                                    float brush = 1.0f - Math.Clamp(dist * (1.0f / brushSize), 0.0f, 1.0f);

                                    int i = (y * 65 + x) * 4;

                                    // Fixed mode
                                    //byte offs = (byte)(brush * 255);
                                    //subchunk.blendMap[i + this.layer] = offs;

                                    byte offs = (byte)(brush * 20);

                                    // Calculate weights
                                    for (int l = 0; l < 4; l++)
                                    {
                                        perc[l] = sc.subArea.blendMap[i + l] / 255f;
                                    }

                                    float added = offs;// - subchunk.blendMap[i + this.layer];

                                    if (sc.subArea.blendMap[i + this.layer] + offs >= 255)
                                        sc.subArea.blendMap[i + this.layer] = 255;
                                    else
                                        sc.subArea.blendMap[i + this.layer] += offs;


                                    // Redistribute weights
                                    for (int k = 0; k < 4; k++)
                                    {
                                        if (k != this.layer)
                                        {
                                            sc.subArea.blendMap[i + k] -= (byte)(added * perc[k]);
                                        }
                                    }

                                    /*
                                    // Accumulate mode
                                    byte offs = (byte)(brush * 10);
                                    if (subchunk.blendMap[i + this.layer] + offs >= 255)
                                        subchunk.blendMap[i + this.layer] = 255;
                                    else
                                        subchunk.blendMap[i + this.layer] += offs;
                                    */
                                }
                            }

                            sc.terrainMaterial.UpdateBlendMap(sc.subArea.blendMap);
                            /*

                            for (int v = 0; v < subchunk.mesh.vertices.Length; v++)
                            {
                                float dist = Vector2.Distance(subchunk.mesh.vertices[v].position.Xz + subPos, hitPoint.Xz);
                                float brush = 1.0f - Math.Clamp(dist * (1.0f / brushSize), 0.0f, 1.0f);

                                subchunk.mesh.vertices[v].position.Y += brush;
                            }

                            subchunk.mesh.ReBuild();
                            */
                        }
                    }
                }
            }
        }

        public override void OnTooboxPaneLoaded()
        {
            
        }
    }
}
