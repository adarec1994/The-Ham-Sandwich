namespace ProjectWS.Engine.Database.Definitions
{
	public class Quest2Reward : TblRecord
	{
		public override string GetFileName() => "Quest2Reward";
		public override uint GetID() => this.ID;
		
		public uint ID;
		public uint quest2Id;
		public uint quest2RewardTypeId;
		public uint objectId;
		public uint objectAmount;
		public uint flags;
	}
}
