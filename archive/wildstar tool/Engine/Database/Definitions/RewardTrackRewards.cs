namespace ProjectWS.Engine.Database.Definitions
{
	public class RewardTrackRewards : TblRecord
	{
		public override string GetFileName() => "RewardTrackRewards";
		public override uint GetID() => this.ID;
		
		public uint ID;
		public uint rewardTrackId;
		public uint rewardPointFlags;
		public uint prerequisiteId;
		public uint flags;
		public uint currencyTypeId;
		public uint currencyAmount;
		public uint rewardTrackRewardTypeEnum00;
		public uint rewardTrackRewardTypeEnum01;
		public uint rewardTrackRewardTypeEnum02;
		public uint rewardChoiceId00;
		public uint rewardChoiceId01;
		public uint rewardChoiceId02;
		public uint rewardChoiceCount00;
		public uint rewardChoiceCount01;
		public uint rewardChoiceCount02;
	}
}
