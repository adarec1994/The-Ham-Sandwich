namespace ProjectWS.Engine.Database.Definitions
{
	public class WorldZone : TblRecord
	{
		public override string GetFileName() => "WorldZone";
		public override uint GetID() => this.ID;
		
		public uint ID;
		public uint localizedTextIdName;
		public uint parentZoneId;
		public uint allowAccess;
		public uint color;
		public uint soundZoneKitId;
		public uint worldLocation2IdExit;
		public uint flags;
		public uint zonePvpRulesEnum;
		public uint rewardRotationContentId;
	}
}
