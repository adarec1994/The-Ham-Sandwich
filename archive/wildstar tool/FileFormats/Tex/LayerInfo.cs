using System.Collections;
using System.Collections.Generic;
using System.IO;

namespace ProjectWS.FileFormats.Tex
{
    public struct LayerInfo
    {
        public sbyte quality;           // 0 - 100
        public sbyte hasReplacement;    // 0 - 1
        public sbyte replacement;       // 0 - 255

        public LayerInfo()
        {
            this.quality = 0;
            this.hasReplacement = 0;
            this.replacement = 0;
        }

        public LayerInfo(BinaryReader br)
        {
            this.quality = br.ReadSByte();
            this.hasReplacement = br.ReadSByte();
            this.replacement = br.ReadSByte();
        }

        public void Write(BinaryWriter bw)
        {
            bw.Write(this.quality);
            bw.Write(this.hasReplacement);
            bw.Write(this.replacement);
        }
    }
}