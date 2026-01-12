namespace ProjectWS.Engine.Database.Definitions
{
	public class MapZoneWorldJoin : TblRecord
	{
		public override string GetFileName() => "MapZoneWorldJoin";
		public override uint GetID() => this.ID;
		
		public uint ID;
		public uint mapZoneId;
		public uint worldId;
	}
}
