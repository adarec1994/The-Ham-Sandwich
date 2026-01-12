namespace ProjectWS.Engine.Database.Definitions
{
	public class RewardProperty : TblRecord
	{
		public override string GetFileName() => "RewardProperty";
		public override uint GetID() => this.ID;
		
		public uint ID;
		public uint localizedTextId;
		public uint rewardModifierValueTypeEnum;
	}
}
