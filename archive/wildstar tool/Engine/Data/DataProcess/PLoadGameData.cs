using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace ProjectWS.Engine.Data.DataProcess
{
    internal class PLoadGameData : Process
    {
        public PLoadGameData(SharedProcessData sharedProcessData) : base(sharedProcessData)
        {

        }

        public override string? Info()
        {
            return "Loading Game Data";
        }

        public override void Run()
        {
            LogProgress(0, Info());

            if (this.sharedProcessData != null && this.sharedProcessData.engineRef != null && this.sharedProcessData.gameClientPath != null)
            {
                this.sharedProcessData.gameData = new GameData(this.sharedProcessData.engineRef, this.sharedProcessData.gameClientPath);
                this.sharedProcessData.gameData.Read(false, LogProgress);
            }
        }

        public override int Total()
        {
            return 3;
        }
    }
}
