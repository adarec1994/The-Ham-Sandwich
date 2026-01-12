namespace ProjectWS.Engine.Database.Definitions
{
	public class MatchingRandomReward : TblRecord
	{
		public override string GetFileName() => "MatchingRandomReward";
		public override uint GetID() => this.ID;
		
		public uint ID;
		public uint matchTypeEnum;
		public uint item2Id;
		public uint itemCount;
		public uint currencyTypeId;
		public uint currencyValue;
		public uint xpEarned;
		public uint item2IdLevelScale;
		public uint itemCountLevelScale;
		public uint currencyTypeIdLevelScale;
		public uint currencyValueLevelScale;
		public uint xpEarnedLevelScale;
		public uint worldDifficultyEnum;
	}
}
