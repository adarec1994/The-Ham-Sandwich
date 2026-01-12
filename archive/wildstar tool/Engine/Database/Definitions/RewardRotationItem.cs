namespace ProjectWS.Engine.Database.Definitions
{
	public class RewardRotationItem : TblRecord
	{
		public override string GetFileName() => "RewardRotationItem";
		public override uint GetID() => this.ID;
		
		public uint ID;
		public uint rewardItemTypeEnum;
		public uint rewardItemObject;
		public uint count;
		public string iconPath;
		public uint minPlayerLevel;
		public uint worldDifficultyFlags;
	}
}
