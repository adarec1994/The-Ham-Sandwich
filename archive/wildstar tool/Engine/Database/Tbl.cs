using ProjectWS.FileFormats.Extensions;

namespace ProjectWS.Engine.Database
{
    public class Tbl<T> where T : TblRecord, new()
    {
        public const int HEADER_SIZE = 96;

        public Dictionary<uint, T> records;
        public uint[] keys;
        public string name;
        public string path;
        public Header header;
        public long headerEndOffset;
        public string internalName;
        public Column[] columns;
        public List<int> lookup;

        public Tbl(string name, Data.GameData data)
        {
            this.name = name;
            this.path = $"DB\\{name}.tbl";
            this.records = new Dictionary<uint, T>();
            this.lookup = new List<int>();

            using (Stream ms = data.GetFileData(this.path))
            {
                using (BinaryReader br = new BinaryReader(ms))
                {
                    Read(br);
                }
            }
        }

        public Tbl(string path)
        {
            this.name = Path.GetFileNameWithoutExtension(path);
            this.path = path;
            this.records = new Dictionary<uint, T>();
            this.lookup = new List<int>();

            using (Stream ms = File.OpenRead(this.path))
            {
                using (BinaryReader br = new BinaryReader(ms))
                {
                    Read(br);
                }
            }
        }

        void Read(BinaryReader br)
        {
            FieldCache[] fieldCache = FieldsCache<T>.Cache;

            this.header = new Header(br);
            this.headerEndOffset = br.BaseStream.Position;
            this.internalName = br.ReadString((int)this.header.tableNameLength, true);
            this.columns = new Column[this.header.fieldCount];
            for (int i = 0; i < this.header.fieldCount; i++)
            {
                br.BaseStream.Position = this.header.fieldOffset + HEADER_SIZE + (24 * i);
                this.columns[i] = new Column(br, this.header);
            }
            long offset = this.header.recordOffset + HEADER_SIZE;
            this.keys = new uint[this.header.recordCount];
            for (uint i = 0; i < this.header.recordCount; i++)
            {
                br.BaseStream.Position = offset + (this.header.recordSize * i);

                T rec = new T();
                rec.Read(fieldCache, rec, br, offset);
                uint id = rec.GetID();
                this.keys[i] = id;
                records.Add(id, rec);
            }

            br.BaseStream.Position = this.header.lookupOffset + HEADER_SIZE;
            for (int i = 0; i < this.header.maxID; i++)
            {
                this.lookup.Add(br.ReadInt32());
            }
        }

        void Write(BinaryWriter bw)
        {
            // Write empty header
            bw.Write(new byte[HEADER_SIZE]);

            // Write table name
            bw.WriteWString(this.internalName);

            // Write columns
            bw.MoveTo(this.header.fieldOffset + HEADER_SIZE);
            for (int i = 0; i < this.columns.Length; i++)
            {
                this.columns[i].Write(bw);
            }

            // Write column names
            var nameStartPos = bw.BaseStream.Position;
            for (int i = 0; i < this.columns.Length; i++)
            {
                this.columns[i].WriteNameData(bw, this.header, nameStartPos);
            }

            // Write data
            FieldCache[] fieldCache = FieldsCache<T>.Cache;
            long recordStartPos = this.header.recordOffset + HEADER_SIZE;
            long stringDataStart = recordStartPos + this.header.recordSize * this.records.Count;

            Dictionary<string, int> stringOffsets = new Dictionary<string, int>();
            long strOffs = 0;

            for (uint i = 0; i < this.lookup.Count; i++)
            {
                if (this.lookup[(int)i] == -1)
                    continue;

                bw.MoveTo(recordStartPos + (this.header.recordSize * this.lookup[(int)i]));

                var rec = this.records[i];
                rec.Write(fieldCache, rec, bw, recordStartPos, stringDataStart, ref stringOffsets, ref strOffs);
            }

            bw.MoveTo(stringDataStart + strOffs);
            bw.Align(16);

            long stringDataEnd = bw.BaseStream.Position;

            // Write lookups
            bw.MoveTo(stringDataEnd);
            for (int i = 0; i < this.lookup.Count; i++)
            {
                bw.Write(this.lookup[i]);
            }

            bw.Write(new byte[8]);

            // Calculate new header offsets
            this.header.recordCount = (uint)this.records.Count;
            this.header.totalRecordSize = strOffs + this.header.recordSize * this.header.recordCount;
            this.header.lookupOffset = stringDataEnd - HEADER_SIZE;
            this.header.maxID = this.lookup.Count;

            // Write header
            bw.BaseStream.Position = 0;
            this.header.Write(bw);
        }

        public class Header
        {
            public int magic;
            public int version;
            public uint tableNameLength;
            public Int64 unk0;
            public uint recordSize;
            public Int64 fieldCount;
            public Int64 fieldOffset;
            public uint recordCount;
            public Int64 totalRecordSize;
            public Int64 recordOffset;
            public Int64 maxID;
            public Int64 lookupOffset;

            public Header(BinaryReader br)
            {
                this.magic = br.ReadInt32();
                this.version = br.ReadInt32();
                this.tableNameLength = br.ReadUInt32();
                br.BaseStream.Position += 4;            // Padding
                this.unk0 = br.ReadInt64();
                this.recordSize = br.ReadUInt32();
                br.BaseStream.Position += 4;            // Padding
                this.fieldCount = br.ReadInt64();
                this.fieldOffset = br.ReadInt64();
                this.recordCount = br.ReadUInt32();
                br.BaseStream.Position += 4;            // Padding
                this.totalRecordSize = br.ReadInt64();
                this.recordOffset = br.ReadInt64();
                this.maxID = br.ReadInt64();
                this.lookupOffset = br.ReadInt64();
                br.BaseStream.Position += 8;            // Padding
            }

            public void Write(BinaryWriter bw)
            {
                bw.Write(this.magic);
                bw.Write(this.version);
                bw.Write((long)this.tableNameLength);
                bw.Write(this.unk0);
                bw.Write((long)this.recordSize);
                bw.Write(this.fieldCount);
                bw.Write(this.fieldOffset);
                bw.Write((long)this.recordCount);
                bw.Write(this.totalRecordSize);
                bw.Write(this.recordOffset);
                bw.Write(this.maxID);
                bw.Write(this.lookupOffset);
                bw.Write((long)0);
            }
        }

        public struct Column
        {
            public uint nameLength;
            public uint unk0;
            public long nameOffset;
            public DataType dataType;
            public uint unk1;
            public string name;

            public Column(BinaryReader br, Header header)
            {
                this.nameLength = br.ReadUInt32();
                this.unk0 = br.ReadUInt32();
                this.nameOffset = br.ReadInt64();
                this.dataType = (DataType)br.ReadUInt16();
                br.BaseStream.Position += 2;        // padding
                this.unk1 = br.ReadUInt32();
                long offset = (long)(HEADER_SIZE + header.fieldCount * 24 + header.fieldOffset + this.nameOffset);
                br.BaseStream.Position = ((header.fieldCount % 2L == 0) ? offset : (offset + 8));
                this.name = br.ReadString((int)(this.nameLength - 1), true);
            }

            internal void Write(BinaryWriter bw)
            {
                bw.Write(this.nameLength);
                bw.Write(this.unk0);
                bw.Write(this.nameOffset);
                bw.Write((uint)this.dataType);
                bw.Write((int)this.unk1);
            }

            internal void WriteNameData(BinaryWriter bw, Header header, long startOffs)
            {
                long offset = startOffs + this.nameOffset;
                bw.MoveTo((header.fieldCount % 2L == 0) ? offset : (offset + 8));
                bw.WriteWString(this.name);
            }

            public enum DataType
            {
                Uint = 3,
                Float = 4,
                Flags = 11,
                Ulong = 20,
                String = 130
            }
        }

        public static Tbl<T> Open(Data.GameData data)
        {
            try
            {
                T table = new T();
                string fileName = table.GetFileName();
                var r = new Tbl<T>(fileName, data);
                return r;
            }
            catch (Exception e)
            {
                Debug.LogException(e);
                return null;
            }
        }

        public static Tbl<T> Open(string path)
        {
            try
            {
                T table = new T();
                //string fileName = table.GetFileName();
                var r = new Tbl<T>(path);
                return r;
            }
            catch (Exception e)
            {
                Debug.LogException(e);
                return null;
            }
        }

        public T Get(uint id)
        {
            this.records.TryGetValue(id, out T rec);
            return rec;
        }

        public void Print()
        {
            /*
            foreach (KeyValuePair<uint, T> item in this.records)
            {
                Debug.Log(JsonConvert.SerializeObject(item.Value, Formatting.Indented));
            }
            */
        }

        public void Add(T rec, uint ID)
        {
            this.records.Add(ID, rec);
            this.lookup.Add(this.lookup[this.lookup.Count - 1] + 1);
        }

        public void Write()
        {
            if (File.Exists(this.path))
                File.Delete(this.path);

            using(var str = File.OpenWrite(this.path))
            {
                using(var bw = new BinaryWriter(str))
                {
                    Write(bw);
                }
            }
        }
    }
}