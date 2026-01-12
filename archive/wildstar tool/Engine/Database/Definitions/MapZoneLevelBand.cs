namespace ProjectWS.Engine.Database.Definitions
{
	public class MapZoneLevelBand : TblRecord
	{
		public override string GetFileName() => "MapZoneLevelBand";
		public override uint GetID() => this.ID;
		
		public uint ID;
		public uint mapZoneHexGroupId;
		public uint levelMin;
		public uint levelMax;
		public uint labelX;
		public uint labelZ;
	}
}
