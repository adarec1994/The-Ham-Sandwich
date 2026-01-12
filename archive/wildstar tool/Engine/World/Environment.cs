using ProjectWS.Engine.Data;

namespace ProjectWS.Engine.World
{
    public class Environment
    {
        World world;
        uint[,] currentSkyIDs = new uint[,]
        { 
            { 0, 0, 0, 0 },
            { 0, 0, 0, 0 },
            { 0, 0, 0, 0 },
            { 0, 0, 0, 0 }
        };

        public Environment(World world)
        {
            this.world = world;
        }

        public void SubchunkChange()
        {
            // Verify current sky id's versus new sky id's
            Chunk? chunk;

            if (this.world == null || this.world.controller == null) return;
            
            this.world.chunks.TryGetValue(this.world.controller.chunkPosition, out chunk);

            if (chunk == null) return;
            if (chunk.area == null) return;

            if (this.world.controller.subchunkIndex >= 0)
            {
                if (chunk.area.subAreas?.Count > this.world.controller.subchunkIndex)
                {
                    var subchunk = chunk.area.subAreas[this.world.controller.subchunkIndex];
                    //var pos = subchunk.centerPosition;
                    //this.world.subchunkGizmo.transform.SetPosition(new Vector3(pos));
                    //this.world.subchunkGizmo.transform.SetScale(Vector3.One * 32.0f);

                    if (subchunk.skyCorners != null)
                    {
                        for (int i = 0; i < 4; i++)
                        {
                            for (int j = 0; j < 4; j++)
                            {
                                if (subchunk.skyCorners[i].worldSkyIDs[j] != this.currentSkyIDs[i, j])
                                {
                                    //SwapSkyChunk(subchunk);
                                    return;
                                }
                            }
                        }
                    }
                }
            }
        }

        void SwapSkyChunk(FileFormats.Area.SubArea subchunk)
        {
            Debug.Log("------------------------------");
            for (int i = 0; i < 4; i++)
            {
                for (int j = 0; j < 4; j++)
                {
                    this.currentSkyIDs[i, j] = subchunk.skyCorners[i].worldSkyIDs[j];

                    if (this.currentSkyIDs[i, j] != 0)
                    {
                        var worldSkyRecord = DataManager.database.worldSky.Get(this.currentSkyIDs[i, j]);
                        if (worldSkyRecord != null)
                        {
                            Debug.Log(worldSkyRecord.assetPath);
                        }
                        else
                        {
                            Debug.LogWarning("Missing world sky record ID : " + this.currentSkyIDs[i, j]);
                        }
                    }
                }
            }
        }
    }
}
