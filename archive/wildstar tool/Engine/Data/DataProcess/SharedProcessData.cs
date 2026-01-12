using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace ProjectWS.Engine.Data.DataProcess
{
    public class SharedProcessData
    {
        public Engine? engineRef;
        public BackgroundWorker? workerRef;
        public AssetDatabase? assetDB;
        public string? gameClientPath;
        public string? assetDBFolder;
        public GameData? gameData;
        public string? extractPath;
    }
}
