using MathUtils;
using ProjectWS.FileFormats.Common;


namespace ProjectWS.FileFormats.I3
{
    public class Unk3Data : ArrayData
    {
        public string? name;
        public short[]? arrUnk;

        public override int GetSize()
        {
            return 40;
        }

        public override void Read(BinaryReader br, long endOffset = 0)
        {
            br.BaseStream.Position += 8;
            this.name = new string(new ArrayWChar(br, endOffset).data);
            this.arrUnk = new ArrayInt16(br, endOffset).data;
        }
    }
}
