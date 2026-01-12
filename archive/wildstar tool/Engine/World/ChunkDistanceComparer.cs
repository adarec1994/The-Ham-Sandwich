using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace ProjectWS.Engine.World
{
    public class ChunkDistanceComparer : IComparer<SubChunk>
    {
        public int Compare(SubChunk a, SubChunk b)
        {
            if (a.distanceToCam > b.distanceToCam)
                return 1;
            else
                return -1;
        }
    }
}
