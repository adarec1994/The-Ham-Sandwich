using System;
using System.Collections;
using System.Collections.Generic;
using System.Diagnostics;

namespace ProjectWS.Engine.TaskManager
{
    public class TerrainTask : Task
    {
        public World.Chunk chunk;
        public int lod;
        public int quadrant;

        public TerrainTask(World.Chunk chunk, int lod, JobType jobType)
        {
            this.dataType = DataType.Terrain;
            this.jobType = jobType;
            this.chunk = chunk;
            this.lod = lod;
            this.quadrant = -1;
        }

        public override void PerformTask()
        {
            switch (this.jobType)
            {
                case JobType.Read:
                    if (this.lod == 0)
                    {
                        this.chunk.ReadArea(this.chunk.area);

                        TerrainTask q0 = new TerrainTask(this.chunk, this.lod, JobType.Build);
                        q0.quadrant = 0;
                        this.chunk.EnqueueBuildTask(q0);
                        TerrainTask q1 = new TerrainTask(this.chunk, this.lod, JobType.Build);
                        q1.quadrant = 1;
                        this.chunk.EnqueueBuildTask(q1);
                        TerrainTask q2 = new TerrainTask(this.chunk, this.lod, JobType.Build);
                        q2.quadrant = 2;
                        this.chunk.EnqueueBuildTask(q2);
                        TerrainTask q3 = new TerrainTask(this.chunk, this.lod, JobType.Build);
                        q3.quadrant = 3;
                        this.chunk.EnqueueBuildTask(q3);
                    }
                    else if (this.lod == 1)
                    {
                        this.chunk.ReadAreaLow(this.chunk.areaLow);

                        TerrainTask q0 = new TerrainTask(this.chunk, this.lod, JobType.Build);
                        q0.quadrant = this.chunk.GetLoDQuadrant(0);
                        this.chunk.EnqueueBuildTask(q0);
                        TerrainTask q1 = new TerrainTask(this.chunk, this.lod, JobType.Build);
                        q1.quadrant = this.chunk.GetLoDQuadrant(1);
                        this.chunk.EnqueueBuildTask(q1);
                        TerrainTask q2 = new TerrainTask(this.chunk, this.lod, JobType.Build);
                        q2.quadrant = this.chunk.GetLoDQuadrant(2);
                        this.chunk.EnqueueBuildTask(q2);
                        TerrainTask q3 = new TerrainTask(this.chunk, this.lod, JobType.Build);
                        q3.quadrant = this.chunk.GetLoDQuadrant(3);
                        this.chunk.EnqueueBuildTask(q3);
                    }
                    break;
                case JobType.Write:
                    break;
                case JobType.Build:
                    this.chunk.Build(this.lod, this.quadrant);
                    break;
                case JobType.Destroy:
                    break;
                default:
                    break;
            }
        }
    }
}