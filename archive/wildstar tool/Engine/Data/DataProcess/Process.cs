using System;
using System.Collections.Concurrent;
using System.Collections.Generic;
using System.ComponentModel;
using System.Diagnostics;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using static ProjectWS.Engine.Data.Archive.Index;

namespace ProjectWS.Engine.Data.DataProcess
{
    public abstract class Process
    {
        public SharedProcessData? sharedProcessData;
        int progress = 0;


        public Process(SharedProcessData sharedProcessData)
        {
            this.sharedProcessData = sharedProcessData;
        }

        public abstract void Run();

        /// <summary>
        /// Returns a description of the process
        /// </summary>
        /// <returns></returns>
        public abstract string? Info();
        /// <summary>
        /// Returns the total number of elements in the process
        /// Used for calculating percentage
        /// </summary>
        /// <returns></returns>
        public abstract int Total();

        public void LogProgress(int percentProgress)
        {
            int idx = (int)((float)percentProgress / (float)Total() * 100f);
            this.sharedProcessData?.workerRef?.ReportProgress(idx);
        }

        public void LogProgress(int percentProgress, string? message)
        {
            int idx = (int)((float)percentProgress / (float)Total() * 100f);
            this.sharedProcessData?.workerRef?.ReportProgress(idx, message);
        }
    }
}
