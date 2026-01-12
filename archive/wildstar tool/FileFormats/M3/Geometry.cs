using ProjectWS.FileFormats.Common;
using System.Runtime.InteropServices;

namespace ProjectWS.FileFormats.M3
{
    public class Geometry : ArrayData
    {
        public long signature;
        public uint vertexBlockCount;
        public ushort vertexBlockSizeInBytes;
        public VertexBlockFlags vertexBlockFlags;
        public VertexFieldType[]? vertexFieldTypes;
        public byte[]? vertexBlockFieldPositions;

        public uint indexCount;
        public byte indexSize;
        public byte unk1;

        public uint[]? indexData;
        public byte[]? vertexData;

        public Submesh[]? submeshes;

        public uint nVertexBlocks;
        public int[]? vertexBlockRange;
        public short[]? unk0A8;
        public int[]? unk0B8;

        private static unsafe byte[] ConvertStruct<T>(ref T str) where T : struct
        {
            int size = Marshal.SizeOf(str);
            var arr = new byte[size];

            fixed (byte* arrPtr = arr)
            {
                Marshal.StructureToPtr(str, (IntPtr)arrPtr, true);
            }

            return arr;
        }

        public override void Read(BinaryReader br, long startOffset)
        {
            this.signature = br.ReadInt64();
            br.BaseStream.Position += 16;       // Unused Array
            this.vertexBlockCount = br.ReadUInt32();
            this.vertexBlockSizeInBytes = br.ReadUInt16();
            this.vertexBlockFlags = (VertexBlockFlags)br.ReadUInt16();

            this.vertexFieldTypes = new VertexFieldType[11];
            for (int i = 0; i < 11; i++)
            {
                this.vertexFieldTypes[i] = (VertexFieldType)br.ReadByte();
            }

            this.vertexBlockFieldPositions = new byte[11];
            for (int i = 0; i < 11; i++)
            {
                this.vertexBlockFieldPositions[i] = br.ReadByte();
            }
            br.BaseStream.Position += 2;        // Padding

            // Read vertex buffer
            this.vertexData = new ArrayByte(br, startOffset).data;

            br.BaseStream.Position += 16;       // Unused Array
            br.BaseStream.Position += 16;       // Unused Array
            this.indexCount = br.ReadUInt32();
            this.indexSize = br.ReadByte();
            this.unk1 = br.ReadByte();
            br.BaseStream.Position += 2;        // Padding

            // Read index buffer
            var arr = new ArrayByte(br, startOffset);
            var indexBuffer = arr.data;
            this.indexData = new uint[this.indexCount];

            using (MemoryStream ms = new MemoryStream(indexBuffer))
            {
                using (BinaryReader bri = new BinaryReader(ms))
                {
                    for (int i = 0; i < this.indexCount; i++)
                    {
                        this.indexData[i] = this.indexSize == 2 ? bri.ReadUInt16() : bri.ReadUInt32();
                    }
                }
            }

            this.submeshes = new ArrayStruct<Submesh>(br, startOffset).data;
            this.nVertexBlocks = br.ReadUInt32();
            br.BaseStream.Position += 4;        // Padding
            this.vertexBlockRange = new ArrayInt32(br, startOffset).data;
            this.unk0A8 = new ArrayInt16(br, startOffset).data;
            this.unk0B8 = new ArrayInt32(br, startOffset).data;
        }
        /*
        public void Build(M3 m3)
        {
            for (int i = 0; i < this.submeshes.Length; i++)
            {
                submesh.vertexData = new byte[submesh.vertexCount * this.vertexBlockSizeInBytes];
                Array.Copy(this.vertexData, submesh.startVertex * this.vertexBlockSizeInBytes, submesh.vertexData, 0, submesh.vertexCount * this.vertexBlockSizeInBytes);

                submesh.indexData = new uint[submesh.indexCount];
                Array.Copy(this.indexData, submesh.startIndex, submesh.indexData, 0, submesh.indexCount);

                if (submesh.meshGroupID == m3.modelID || submesh.meshGroupID == -1)
                {
                    submesh.Build(this.vertexBlockSizeInBytes, this.vertexBlockFieldPositions, this.vertexBlockFlags, this.vertexFieldTypes);

                    m3.materials[submesh.materialSelector].Build(m3);
                }
            }
        }
        */
        public override int GetSize()
        {
            return 208;
        }

        [Flags]
        public enum VertexBlockFlags : short
        {
            hasPosition = 0x1,
            hasTangent = 0x2,
            hasNormal = 0x4,
            hasBiTangent = 0x8,
            hasBoneIndices = 0x10,
            hasBoneWeights = 0x20,
            hasVertexColor0 = 0x40,
            hasVertexColor1 = 0x80,
            hasUV0 = 0x100,
            hasUV1 = 0x200,
            hasUnknown = 0x400,
        }

        public enum VertexFieldType : byte
        {
            Null = 0,
            Vector3_32bit = 1,
            Vector3_16bit = 2,
            Vector3_8bitNorm = 3,
            Vector4_8bit = 4,
            Vector2_16bit = 5,
            Unk_8bit = 6,
        }

        [StructLayout(LayoutKind.Explicit)]
        struct Fp32
        {
            [FieldOffset(0)]
            public uint u;
            [FieldOffset(0)]
            public float f;
        }

        float Real16ToReal32(ushort input)
        {
            /*
             * https://gist.github.com/rygorous/2144712
             * Public domain, by Fabian "ryg" Giesen
             */

            Fp32 magic = new Fp32 { u = (254U - 15U) << 23 };
            Fp32 was_infnan = new Fp32 { u = (127U + 16U) << 23 };
            Fp32 output = new Fp32();

            output.u = (input & 0x7FFFU) << 13;   /* exponent/mantissa bits */
            output.f *= magic.f;                  /* exponent adjust */
            if (output.f >= was_infnan.f)         /* make sure Inf/NaN survive */
            {
                output.u |= 255U << 23;
            }
            output.u |= (input & 0x8000U) << 16;  /* sign bit */

            return output.f;
        }
    }
}