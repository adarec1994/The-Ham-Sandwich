using MathUtils;
using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace ProjectWS.Editor.Project
{
    public class MapChunkInfo
    {
        public List<Vector2i>? chunks { get; set; }
        public List<Vector2i>? chunksLow { get; set; }
        public List<Vector2i>? minimaps { get; set; }
    }
}
