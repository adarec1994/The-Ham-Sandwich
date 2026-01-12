using MathUtils;
using ProjectWS.FileFormats.Common;
using ProjectWS.FileFormats.Extensions;

namespace ProjectWS.FileFormats.M3
{
    public partial class Submesh : ArrayData
    {
        public uint startIndex;
        public uint startVertex;
        public uint indexCount;
        public uint vertexCount;
        public ushort boneMapIndex;
        public ushort boneMapCount;
        public ushort unk2;
        public short materialSelector;
        public short unk3;
        public short unk4;
        public short unk5;
        public sbyte meshGroupID;
        public byte unk6;
        public short unk7;
        public BodyPart meshAnatomyID;
        public short unk8;
        public short unk9;
        public short unk10;
        public short unk11;
        public short unk12;
        public short unk13;
        public Color color0;
        public Color color1;
        public byte unk16;
        public byte unk17;

        public byte[]? pad;

        public Vector3 boundsMin;
        public Vector3 boundsMax;
        public Vector3 size;
        public Vector3 offset;
        public Vector3 unk18;

        public override void Read(BinaryReader br, long startOffset)
        {
            this.startIndex = br.ReadUInt32();
            this.startVertex = br.ReadUInt32();
            this.indexCount = br.ReadUInt32();
            this.vertexCount = br.ReadUInt32();
            this.boneMapIndex = br.ReadUInt16();
            this.boneMapCount = br.ReadUInt16();
            this.unk2 = br.ReadUInt16();
            this.materialSelector = br.ReadInt16();
            this.unk3 = br.ReadInt16();
            this.unk4 = br.ReadInt16();
            this.unk5 = br.ReadInt16();
            this.meshGroupID = br.ReadSByte();
            this.unk6 = br.ReadByte();
            this.unk7 = br.ReadInt16();
            this.meshAnatomyID = (BodyPart)br.ReadInt16();
            this.unk8 = br.ReadInt16();
            this.unk9 = br.ReadInt16();
            this.unk10 = br.ReadInt16();
            this.unk11 = br.ReadInt16();
            this.unk12 = br.ReadInt16();
            this.unk13 = br.ReadInt16();
            this.color0 = br.ReadColor32();
            this.color1 = br.ReadColor32();
            this.unk16 = br.ReadByte();
            this.unk17 = br.ReadByte();
            br.BaseStream.Position += 6;        // Padding
            this.boundsMin = new Vector3(br.ReadSingle(), br.ReadSingle(), br.ReadSingle()); br.ReadSingle(); // skip W
            this.boundsMax = new Vector3(br.ReadSingle(), br.ReadSingle(), br.ReadSingle()); br.ReadSingle(); // skip W
            this.unk18 = new Vector3(br.ReadSingle(), br.ReadSingle(), br.ReadSingle()); br.ReadSingle(); // skip W
            this.size = Vector3.One * (this.boundsMax.Y - this.boundsMin.Y);//new Vector3(Mathf.Abs(this.boundsMax.x - this.boundsMin.x), Mathf.Abs(this.boundsMax.y - this.boundsMin.y), Mathf.Abs(this.boundsMax.z - this.boundsMin.z));
        }

        public override int GetSize()
        {
            return 112;
        }
    }
}