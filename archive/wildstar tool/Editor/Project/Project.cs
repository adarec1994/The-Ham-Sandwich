using MathUtils;
using ProjectWS.Engine.Database;
using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace ProjectWS.Engine.Project
{
    public class Project
    {
        public Guid UUID { get; set; }
        public string? Name { get; set; }
        public List<Map>? Maps { get; set; }
        public uint lastWorldID { get; set; }
        public uint lastWorldLocationID { get; set; }
        public uint previousOpenMapID { get; set; }

        public class Map
        {
            public string? Name { get; set; }
            public World? worldRecord { get; set; }
            public bool isGameMap { get; set; }
            public string? mapChunkInfoPath { get; set; }
            public Vector3 lastPosition { get; set; }
            public Quaternion lastOrientation { get; set; }

            public class World
            {
                public uint ID { get; set; }
                public string? assetPath { get; set; }
                public uint flags { get; set; }
                public uint type { get; set; }
                public string? screenPath { get; set; }
                public string? screenModelPath { get; set; }
                public uint chunkBounds00 { get; set; }
                public uint chunkBounds01 { get; set; }
                public uint chunkBounds02 { get; set; }
                public uint chunkBounds03 { get; set; }
                public uint plugAverageHeight { get; set; }
                public uint localizedTextIdName { get; set; }
                public uint minItemLevel { get; set; }
                public uint maxItemLevel { get; set; }
                public uint primeLevelOffset { get; set; }
                public uint primeLevelMax { get; set; }
                public uint veteranTierScalingType { get; set; }
                public uint heroismMenaceLevel { get; set; }
                public uint rewardRotationContentId { get; set; }
            }
        }

        public Project()
        {
            this.Maps = new List<Map>();
        }
    }
}
