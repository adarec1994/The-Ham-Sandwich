using MathUtils;
using ProjectWS.FileFormats.Extensions;

namespace ProjectWS.FileFormats.Sky
{
    public class AngleABAndColor
    {
        public Vector4 angleA;   // Euler
        public Vector4 color;
        public Vector4 angleB;   // Euler

        public AngleABAndColor(BinaryReader br)
        {
            this.angleA = br.ReadVector4();
            this.color = br.ReadColor();
            this.angleB = br.ReadVector4();
        }

        public override string ToString()
        {
            return $"{this.angleA} {this.color} {this.angleB}";
        }
    }

}
