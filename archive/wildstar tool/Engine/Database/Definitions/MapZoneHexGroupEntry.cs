namespace ProjectWS.Engine.Database.Definitions
{
	public class MapZoneHexGroupEntry : TblRecord
	{
		public override string GetFileName() => "MapZoneHexGroupEntry";
		public override uint GetID() => this.ID;
		
		public uint ID;
		public uint mapZoneHexGroupId;
		public uint hexX;
		public uint hexY;
	}
}
