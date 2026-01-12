namespace ProjectWS.Engine.Database.Definitions
{
	public class PublicEvent : TblRecord
	{
		public override string GetFileName() => "PublicEvent";
		public override uint GetID() => this.ID;
		
		public uint ID;
		public uint worldId;
		public uint worldZoneId;
		public uint localizedTextIdName;
		public uint failureTimeMs;
		public uint worldLocation2Id;
		public uint publicEventTypeEnum;
		public uint publicEventIdParent;
		public uint minPlayerLevel;
		public uint liveEventIdLifetime;
		public uint publicEventFlags;
		public uint localizedTextIdEnd;
		public uint rewardRotationContentId;
	}
}
