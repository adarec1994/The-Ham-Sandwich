namespace ProjectWS.Engine.Database.Definitions
{
	public class RewardRotationEssence : TblRecord
	{
		public override string GetFileName() => "RewardRotationEssence";
		public override uint GetID() => this.ID;
		
		public uint ID;
		public uint accountCurrencyTypeId;
		public uint minPlayerLevel;
		public uint worldDifficultyFlags;
	}
}
