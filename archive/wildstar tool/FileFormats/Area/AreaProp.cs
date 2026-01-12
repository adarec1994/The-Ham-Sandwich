using MathUtils;
using ProjectWS.FileFormats.Extensions;
using System.Text.Json.Serialization;

namespace ProjectWS.FileFormats.Area
{
    public class AreaProp
    {
        public uint uniqueID { get; }
        public uint someID { get; set; }
        public int unk0 { get; set; }
        public int unk1 { get; set; }
        public ModelType modelType { get; }
        public int nameOffset;
        public int unkOffset;
        public float scale { get; set; }
        public Quaternion rotation { get; set; }
        public Vector3 position { get; set; }
        public Placement placement { get; }
        public int unk7 { get; set; }
        public int unk8 { get; set; }
        public int unk9 { get; set; }
        public Color32 color0 { get; set; }
        public Color32 color1 { get; set; }
        public int unk10 { get; set; }
        public int unk11 { get; set; }
        public Color32 color2 { get; set; }
        public int unk12 { get; set; }
        public string? path { get; }

        [JsonIgnore]
        public bool loadRequested;

        public AreaProp(uint uuid, string path)
        {
            this.uniqueID = uuid;
            this.path = path;

            this.placement = new Placement(0, 0, 3000, 3000);
        }

        public AreaProp(BinaryReader br, long chunkStart)
        {
            this.uniqueID = br.ReadUInt32();
            this.someID = br.ReadUInt32();
            this.unk0 = br.ReadInt32();
            this.unk1 = br.ReadInt32();
            this.modelType = (ModelType)br.ReadInt32();
            this.nameOffset = br.ReadInt32();
            this.unkOffset = br.ReadInt32();
            this.scale = br.ReadSingle();
            this.rotation = br.ReadQuaternion(true);
            this.position = br.ReadVector3();
            this.placement = new Placement(br);
            this.unk7 = br.ReadInt32();
            this.unk8 = br.ReadInt32();
            this.unk9 = br.ReadInt32();
            this.color0 = br.ReadColor32();
            this.color1 = br.ReadColor32();
            this.unk10 = br.ReadInt32();
            this.unk11 = br.ReadInt32();
            this.color2 = br.ReadColor32();
            this.unk12 = br.ReadInt32();

            this.loadRequested = false;

            if (this.nameOffset != 0)
            {
                long save = br.BaseStream.Position;
                br.BaseStream.Position = chunkStart + this.nameOffset;
                this.path = br.ReadWString();
                br.BaseStream.Position = save;
            }
            else
            {
                this.path = null;
            }
        }

        public void Write(BinaryWriter bw, int propsSize, ref Dictionary<string, uint> names, ref uint lastNameOffset)
        {
            bw.Write(this.uniqueID);
            bw.Write(this.someID);
            bw.Write(this.unk0);
            bw.Write(this.unk1);
            bw.Write((int)this.modelType);
            // Name offset
            if (this.path == null)
            {
                bw.Write(0);
            }
            else
            {
                if (names.ContainsKey(this.path))
                {
                    bw.Write((uint)(names[this.path] + propsSize));
                }
                else
                {
                    names.Add(this.path, lastNameOffset);
                    bw.Write((uint)(lastNameOffset + propsSize));
                    lastNameOffset += (uint)(this.path.Length * 2 + 2);
                }
            }
            bw.Write(this.unkOffset);
            bw.Write(this.scale);
            bw.Write(this.rotation.X);
            bw.Write(this.rotation.Y);
            bw.Write(this.rotation.Z);
            bw.Write(this.rotation.W);
            bw.Write(this.position.X);
            bw.Write(this.position.Y);
            bw.Write(this.position.Z);
            bw.Write(this.placement.minX);
            bw.Write(this.placement.minY);
            bw.Write(this.placement.maxX);
            bw.Write(this.placement.maxY);
            bw.Write(this.unk7);
            bw.Write(this.unk8);
            bw.Write(this.unk9);
            bw.Write(this.color0.R);
            bw.Write(this.color0.G);
            bw.Write(this.color0.B);
            bw.Write(this.color0.A);
            bw.Write(this.color1.R);
            bw.Write(this.color1.G);
            bw.Write(this.color1.B);
            bw.Write(this.color1.A);
            bw.Write(this.unk10);
            bw.Write(this.unk11);
            bw.Write(this.color2.R);
            bw.Write(this.color2.G);
            bw.Write(this.color2.B);
            bw.Write(this.color2.A);
            bw.Write(this.unk12);
        }

        public enum ModelType
        {
            M3 = 0,
            I3 = 1,
            Unk_2 = 2,          // Maybe light or volume, doesn't have a file path
            DGN = 3,
            Unk_4 = 4,          // Maybe light or volume, doesn't have a file path
        }
    }
}
