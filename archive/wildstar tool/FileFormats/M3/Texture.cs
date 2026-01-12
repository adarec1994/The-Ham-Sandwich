using ProjectWS.FileFormats.Common;

namespace ProjectWS.FileFormats.M3
{
    public class Texture : ArrayData
    {
        public short unk0;
        public TextureType textureType;
        public byte unk1;
        public int unk2;
        public float unk3;
        public byte[]? unks;
        public string? texturePath;

        public override void Read(BinaryReader br, long endOffset)
        {
            this.unk0 = br.ReadInt16();
            this.textureType = (TextureType)br.ReadByte();
            this.unk1 = br.ReadByte();
            this.unk2 = br.ReadInt32();
            this.unk3 = br.ReadSingle();
            this.unks = br.ReadBytes(4);
            var array = new ArrayWChar(br, endOffset);
            this.texturePath = new string(array.data, 0, (int)array.elements - 1);
        }

        public override int GetSize()
        {
            return 32;
        }

        public enum TextureType : byte
        {
            Diffuse = 0,
            Normal = 1,
            FX = 2,
        }
    }
}