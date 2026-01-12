using MathUtils;
using ProjectWS.FileFormats.Extensions;

namespace ProjectWS.FileFormats.Sky
{
    public class Gradient16
    {
        public byte[] padding;
        public Vector4[] colors;

        public Gradient16(BinaryReader br)
        {
            this.padding = br.ReadBytes(16);
            this.colors = new Vector4[16];
            for (int i = 0; i < 16; i++)
            {
                this.colors[i] = br.ReadColor();
            }
        }
    }

}
