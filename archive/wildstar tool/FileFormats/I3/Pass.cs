using MathUtils;
using ProjectWS.FileFormats.Common;


namespace ProjectWS.FileFormats.I3
{
    public class Pass : ArrayData
    {
        public uint unk;
        public string? name;
        public PassUnk0[]? arrUnk0;
        public short[]? arrUnk1;
        public TextureUnit[]? texUnits;
        public short[]? arrUnk2;        // unused
        
        public override int GetSize()
        {
            return 88;
        }

        public override void Read(BinaryReader br, long endOffset = 0)
        {
            br.BaseStream.Position += 4;
            this.unk = br.ReadUInt32();
            this.name = new string(new ArrayWChar(br, endOffset).data);
            this.arrUnk0 = new ArrayStruct<PassUnk0>(br, endOffset, true).data;
            this.arrUnk1 = new ArrayInt16(br, endOffset).data;
            this.texUnits = new ArrayStruct<TextureUnit>(br, endOffset, true).data;
            this.arrUnk2 = new ArrayInt16(br, endOffset).data;
        }
    }
}
