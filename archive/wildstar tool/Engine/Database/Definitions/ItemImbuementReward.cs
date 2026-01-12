namespace ProjectWS.Engine.Database.Definitions
{
	public class ItemImbuementReward : TblRecord
	{
		public override string GetFileName() => "ItemImbuementReward";
		public override uint GetID() => this.ID;
		
		public uint ID;
		public uint itemImbuementRewardTypeEnum;
		public uint rewardObjectId;
		public uint rewardValue;
		public float rewardValueFloat;
	}
}
