namespace ProjectWS.Engine.Database.Definitions
{
	public class DailyLoginReward : TblRecord
	{
		public override string GetFileName() => "DailyLoginReward";
		public override uint GetID() => this.ID;
		
		public uint ID;
		public uint loginDay;
		public uint dailyLoginRewardTypeEnum;
		public uint rewardObjectValue;
		public uint dailyLoginRewardTierEnum;
	}
}
