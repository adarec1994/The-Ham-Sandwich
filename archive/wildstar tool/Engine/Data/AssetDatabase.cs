using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Text;
using System.Text.Json;
using System.Text.Json.Serialization;
using System.Threading.Tasks;

namespace ProjectWS.Engine.Data
{
    public class AssetDatabase
    {
        public enum DataStatus
        {
            None = 0,
            InProgress = 1,
            Ready = 2,
        }

        public int version { get; set; }
        public DataStatus database { get; set; }
        public DataStatus gameArt { get; set; }
        public DataStatus propThumbnails { get; set; }

        public AssetDatabase()
        {
            database = DataStatus.None;
            gameArt = DataStatus.None;
            propThumbnails = DataStatus.None;
        }
    }
}
