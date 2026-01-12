using MathUtils;
using ProjectWS.FileFormats.Extensions;

namespace ProjectWS.FileFormats.Sky
{
    public class AngleAndColorAB
    {
        public Vector4 angle;   // Euler
        public Vector4 colorA;
        public Vector4 colorB;

        public AngleAndColorAB(BinaryReader br)
        {
            this.angle = br.ReadVector4();
            this.colorA = br.ReadColor();
            this.colorB = br.ReadColor();
        }

        public override string ToString()
        {
            return $"{this.angle} {this.colorA} {this.colorB}";
        }
    }

}
