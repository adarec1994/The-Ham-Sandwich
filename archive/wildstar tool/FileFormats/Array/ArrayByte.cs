using System.Collections;
using System.Collections.Generic;
using System.IO;

namespace ProjectWS.FileFormats.Common
{
    public class ArrayByte : Array<byte>
    {
        public ArrayByte(BinaryReader br, long startOffset)
        {
            long save = ReadArrayCommon(br, startOffset);

            // Read actual data
            this.data = br.ReadBytes((int)this.elements);

            br.BaseStream.Position = save;
        }
    }
}