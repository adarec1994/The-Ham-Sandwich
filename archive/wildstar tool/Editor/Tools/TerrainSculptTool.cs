using ProjectWS.Engine.Rendering;
using System;
using MathUtils;

namespace ProjectWS.Editor.Tools
{
    public class TerrainSculptTool : Tool
    {
        readonly Engine.Engine engine;
        public readonly WorldRenderer? worldRenderer;
        readonly Editor editor;

        public Mode mode = Mode.RaiseLower;

        public enum Mode
        {
            RaiseLower,
            Flatten
        }

        public TerrainSculptTool(Engine.Engine engine, Editor editor, WorldRenderer world)
        {
            this.hasBrush = true;
            this.editor = editor;
            this.engine = engine;
            this.worldRenderer = world;
            this.isEnabled = false;
        }

        public override void Enable()
        {
            this.isEnabled = true;
            if (this.worldRenderer != null)
            {
                if (this.worldRenderer.brushParameters != null)
                    this.worldRenderer.brushParameters.mode = Engine.Rendering.ShaderParams.BrushParameters.BrushMode.Gradient;
                if (this.worldRenderer.mousePick != null)
                    this.worldRenderer.mousePick.mode = Engine.MousePick.Mode.Terrain;
            }

            this.mode = Tools.TerrainSculptTool.Mode.RaiseLower;
        }

        public override void Disable()
        {
            this.isEnabled = false;
            if (this.worldRenderer != null && this.worldRenderer.mousePick != null)
                this.worldRenderer.mousePick.mode = Engine.MousePick.Mode.Disabled;
        }

        public override void Update(float deltaTime)
        {
            float flat = ((8400 & 0x7FFF) / 8.0f) - 2048.0f;
            if (this.worldRenderer == null) return;
            if (this.worldRenderer.world == null)  return;
            if (this.worldRenderer.mousePick == null) return;

            // Update brush size
            bool brushEnabled = false;
            float brushSize = 0.0f;
            if (this.worldRenderer.brushParameters != null)
            {
                brushEnabled = this.worldRenderer.brushParameters.isEnabled;
                brushSize = this.worldRenderer.brushParameters.size;
                this.worldRenderer.brushParameters.size += this.engine.input.GetMouseScroll();
                this.worldRenderer.brushParameters.size = (float)Math.Clamp(this.worldRenderer.brushParameters.size, 1.0f, 100f);
            }

            if (this.engine.input.LMB && this.editor.keyboardFocused && brushEnabled)
            {
                var hitPoint = this.worldRenderer.mousePick.terrainHitPoint;
                var terrainAreaHit = this.worldRenderer.mousePick.areaHit;

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

                            var subchunkIndex = s;
                            var subchunk = chunk.subChunks[subchunkIndex];
                            for (int v = 0; v < subchunk.terrainMesh.vertices.Length; v++)
                            {
                                float dist = Vector2.Distance(subchunk.terrainMesh.vertices[v].position.Xz + subPos, hitPoint.Xz);
                                //float dist = Vector2.ManhattanDistance(subchunk.terrainMesh.vertices[v].position.Xz + subPos, hitPoint.Xz);
                                float brush = 1.0f - Math.Clamp(dist * (1.0f / brushSize), 0.0f, 1.0f);

                                if (this.mode == Mode.RaiseLower)
                                {
                                    subchunk.terrainMesh.vertices[v].position.Y += brush;
                                }
                                else if (this.mode == Mode.Flatten)
                                {
                                    // float h = ((heightMap[(y + 1) * 19 + x + 1] & 0x7FFF) / 8.0f) - 2048.0f;
                                    if (subchunk.terrainMesh.vertices[v].position.Y > flat)
                                    {
                                        subchunk.terrainMesh.vertices[v].position.Y -= brush;
                                        if (subchunk.terrainMesh.vertices[v].position.Y < flat)
                                            subchunk.terrainMesh.vertices[v].position.Y = flat;
                                    }
                                    else if (subchunk.terrainMesh.vertices[v].position.Y < flat)
                                    {
                                        subchunk.terrainMesh.vertices[v].position.Y += brush;
                                        if (subchunk.terrainMesh.vertices[v].position.Y > flat)
                                            subchunk.terrainMesh.vertices[v].position.Y = flat;
                                    }
                                }
                            }

                            subchunk.terrainMesh.ReBuild();
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
