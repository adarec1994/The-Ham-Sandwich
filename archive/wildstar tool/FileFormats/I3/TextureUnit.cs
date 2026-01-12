using MathUtils;
using ProjectWS.FileFormats.Common;
using System;
using System.Diagnostics;

namespace ProjectWS.FileFormats.I3
{
    public class TextureUnit : ArrayData
    {
        public short unk0;
        public ushort flags;
        public uint unk1;
        public float unk2;
        public string? texturePath;

        public override int GetSize()
        {
            return 32;
        }

        public override void Read(BinaryReader br, long endOffset = 0)
        {
            this.unk0 = br.ReadInt16();
            this.flags = br.ReadUInt16();
            this.unk1 = br.ReadUInt32();
            this.unk2 = br.ReadSingle();
            br.BaseStream.Position += 4;
            this.texturePath = new string(new ArrayWChar(br, endOffset).data);
        }
    }
}
