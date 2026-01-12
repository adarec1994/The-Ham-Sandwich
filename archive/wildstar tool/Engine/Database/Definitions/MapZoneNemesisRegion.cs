namespace ProjectWS.Engine.Database.Definitions
{
	public class MapZoneNemesisRegion : TblRecord
	{
		public override string GetFileName() => "MapZoneNemesisRegion";
		public override uint GetID() => this.ID;
		
		public uint ID;
		public uint mapZoneHexGroupId;
		public uint localizedTextIdDescription;
		public uint faction2Id;
	}
}
