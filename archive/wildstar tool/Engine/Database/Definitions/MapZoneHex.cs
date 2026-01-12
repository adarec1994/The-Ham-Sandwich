namespace ProjectWS.Engine.Database.Definitions
{
	public class MapZoneHex : TblRecord
	{
		public override string GetFileName() => "MapZoneHex";
		public override uint GetID() => this.ID;
		
		public uint ID;
		public uint mapZoneId;
		public uint pos0;
		public uint pos1;
		public uint flags;
	}
}
