using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.IO;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using static ProjectWS.Engine.Data.Archive.Data;

namespace ProjectWS.Engine.Data.DataProcess
{
    internal class PExtractDatabase : Process
    {
        string gameDataDir = $"{Data.Archive.rootBlockName}\\DB";

        public PExtractDatabase(SharedProcessData spd) : base(spd)
        {

        }

        public override void Run()
        {
            LogProgress(0, Info());

            if (this.sharedProcessData?.gameData != null && this.sharedProcessData?.assetDBFolder != null)
            {
                var fileEntries = this.sharedProcessData.gameData.GetFileEntries(this.gameDataDir);
                var dir = Path.Combine(this.sharedProcessData.assetDBFolder, "DB");
                if (!Directory.Exists(dir))
                    Directory.CreateDirectory(dir);

                int idx = 0;
                foreach (var entry in fileEntries)
                {
                    using (var fs = new System.IO.FileStream(Path.Combine(dir, entry.Key), FileMode.Create, System.IO.FileAccess.Write))
                    using (var ms = this.sharedProcessData.gameData.GetFileData(entry.Value))
                    {
                        ms?.WriteTo(fs);
                        LogProgress(idx++);
                    }
                }
            }
        }

        public override string Info()
        {
            return "Extracting Database.";
        }

        public override int Total()
        {
            if (this.sharedProcessData?.gameData != null)
                return this.sharedProcessData.gameData.GetFileEntries(this.gameDataDir).Count;
            return 384;
        }
    }
}
