namespace ProjectWS.Engine.Database.Definitions
{
	public class RewardRotationModifier : TblRecord
	{
		public override string GetFileName() => "RewardRotationModifier";
		public override uint GetID() => this.ID;
		
		public uint ID;
		public uint rewardPropertyId;
		public uint rewardPropertyData;
		public float value;
		public uint minPlayerLevel;
		public uint worldDifficultyFlags;
	}
}
