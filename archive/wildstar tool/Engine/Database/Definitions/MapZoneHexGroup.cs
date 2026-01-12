namespace ProjectWS.Engine.Database.Definitions
{
	public class MapZoneHexGroup : TblRecord
	{
		public override string GetFileName() => "MapZoneHexGroup";
		public override uint GetID() => this.ID;
		
		public uint ID;
		public uint mapZoneId;
	}
}
