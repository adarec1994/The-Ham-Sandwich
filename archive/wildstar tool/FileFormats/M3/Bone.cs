using MathUtils;
using ProjectWS.FileFormats.Common;

namespace ProjectWS.FileFormats.M3
{
    public class Bone : ArrayData
    {
        public short boneID;                // Entry in ModelBone.tbl
        public Flags flags;                 // Maybe flags
        public short parentId;              // A value of -1 indicates this bone has no parent

        public short unk2;                  // Maybe submesh ID
        public byte[]? unk3;                 // Bone name CRC maybe, or uint16 uDistToFurthDesc and uint16 uZRatioOfChain

        public Track<Vector3>[]? scale;
        public Track<Quaternion>[]? rotation;
        public Track<Vector3>[]? position;

        public Matrix4 bindPose;
        public Matrix4 inverseBindPose;
        public Vector3 pivot;               // The bone pivot in world space

        [Flags]
        public enum Flags : ushort
        {
            unk0x1 = 0x1,
            unk0x2 = 0x2,
            unk0x4 = 0x4,
            unk0x8 = 0x8,
            unk0x10 = 0x10,
            unk0x20 = 0x20,
            unk0x40 = 0x40,
            unk0x80 = 0x80,
            unk0x100 = 0x100,
            unk0x200 = 0x200,
            unk0x400 = 0x400,
            unk0x800 = 0x800,
            unk0x1000 = 0x1000,
            unk0x2000 = 0x2000,
            unk0x4000 = 0x4000,
            unk0x8000 = 0x8000,
        }

        public override void Read(BinaryReader br, long endOffset)
        {
            this.boneID = br.ReadInt16();
            this.flags = (Flags)br.ReadUInt16();
            this.parentId = br.ReadInt16();
            this.unk2 = br.ReadInt16();
            this.unk3 = br.ReadBytes(4);

            br.BaseStream.Position += 4;        // Padding

            this.scale = new Track<Vector3>[4];
            for (int i = 0; i < 4; i++)
            {
                this.scale[i] = new TrackVector3S(br, endOffset);
            }

            this.rotation = new Track<Quaternion>[2];
            for (int i = 0; i < 2; i++)
            {
                this.rotation[i] = new TrackQuaternionS(br, endOffset);
            }

            this.position = new Track<Vector3>[2];
            for (int i = 0; i < 2; i++)
            {
                this.position[i] = new TrackVector3(br, endOffset);
            }

            this.bindPose = new Matrix4
            {
                M11 = br.ReadSingle(),
                M21 = br.ReadSingle(),
                M31 = br.ReadSingle(),
                M41 = br.ReadSingle(),
                M12 = br.ReadSingle(),
                M22 = br.ReadSingle(),
                M32 = br.ReadSingle(),
                M42 = br.ReadSingle(),
                M13 = br.ReadSingle(),
                M23 = br.ReadSingle(),
                M33 = br.ReadSingle(),
                M43 = br.ReadSingle(),
                M14 = br.ReadSingle(),
                M24 = br.ReadSingle(),
                M34 = br.ReadSingle(),
                M44 = br.ReadSingle()
            };

            this.inverseBindPose = new Matrix4
            {
                M11 = br.ReadSingle(),
                M21 = br.ReadSingle(),
                M31 = br.ReadSingle(),
                M41 = br.ReadSingle(),
                M12 = br.ReadSingle(),
                M22 = br.ReadSingle(),
                M32 = br.ReadSingle(),
                M42 = br.ReadSingle(),
                M13 = br.ReadSingle(),
                M23 = br.ReadSingle(),
                M33 = br.ReadSingle(),
                M43 = br.ReadSingle(),
                M14 = br.ReadSingle(),
                M24 = br.ReadSingle(),
                M34 = br.ReadSingle(),
                M44 = br.ReadSingle()
            };

            this.pivot = new Vector3(br.ReadSingle(), br.ReadSingle(), br.ReadSingle());
            br.BaseStream.Position += 4;        // Padding
        }

        public override int GetSize()
        {
            return 352;
        }
    }
}