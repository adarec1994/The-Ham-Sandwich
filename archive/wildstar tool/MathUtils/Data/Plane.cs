using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace MathUtils
{
    public struct Plane
    {
        public Vector3 normal;
        public float d;

        public Plane(BinaryReader br)
        {
            this.normal = new Vector3(br.ReadSingle(), br.ReadSingle(), br.ReadSingle());
            this.d = br.ReadSingle();
        }
    }
}
