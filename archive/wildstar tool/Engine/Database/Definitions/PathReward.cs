namespace ProjectWS.Engine.Database.Definitions
{
	public class PathReward : TblRecord
	{
		public override string GetFileName() => "PathReward";
		public override uint GetID() => this.ID;
		
		public uint ID;
		public uint pathRewardTypeEnum;
		public uint objectId;
		public uint spell4Id;
		public uint item2Id;
		public uint quest2Id;
		public uint characterTitleId;
		public uint prerequisiteId;
		public uint count;
		public uint pathRewardFlags;
		public uint pathScientistScanBotProfileId;
	}
}
