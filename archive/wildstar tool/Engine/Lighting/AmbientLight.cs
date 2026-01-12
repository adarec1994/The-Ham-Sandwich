using OpenTK;
using OpenTK.Graphics;
using MathUtils;
using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace ProjectWS.Engine.Lighting
{
    public class AmbientLight : Light
    {
        public AmbientLight(Vector4 color) => this.color = color;

        public override void ApplyToShader(Shader shader)
        {
            shader.SetColor4("ambientColor", this.color);
        }
    }
}
