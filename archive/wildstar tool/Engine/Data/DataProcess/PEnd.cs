using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace ProjectWS.Engine.Data.DataProcess
{
    internal class PEnd : Process
    {
        public PEnd(SharedProcessData sharedProcessData) : base(sharedProcessData)
        {

        }

        public override string? Info()
        {
            return "Done";
        }

        public override void Run()
        {
            LogProgress(0, Info());
        }

        public override int Total()
        {
            return 0;
        }
    }
}
