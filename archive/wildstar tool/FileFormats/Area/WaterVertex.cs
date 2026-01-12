using MathUtils;
using ProjectWS.FileFormats.Extensions;
using System.Runtime.InteropServices;

namespace ProjectWS.FileFormats.Area
{
    [StructLayout(LayoutKind.Sequential)]
    public struct WaterVertex
    {
        public Vector3 position;
        public Vector3 normal;
        public Vector4 tangent;
        public Vector4 bitangent;
        public Vector2 uv;
        public Vector4 color;
        public float unk0;
        public int unk1;
        public Vector4 layerBlendMask;

        public WaterVertex(BinaryReader br)
        {
            this.position = br.ReadVector3();
            this.normal = br.ReadVector3();
            this.tangent = new Vector4(br.ReadSingle(), br.ReadSingle(), br.ReadSingle(), 0);
            this.bitangent = new Vector4(br.ReadSingle(), br.ReadSingle(), br.ReadSingle(), 0);
            this.uv = new Vector2(br.ReadSingle(), br.ReadSingle());
            this.color = new Vector4(br.ReadByte() / 255f, br.ReadByte() / 255f, br.ReadByte() / 255f, br.ReadByte() / 255f);
            this.unk0 = br.ReadSingle();
            this.unk1 = br.ReadInt32();
            this.layerBlendMask = new Vector4(br.ReadByte() / 255f, br.ReadByte() / 255f, br.ReadByte() / 255f, br.ReadByte() / 255f);
        }
    }
}
