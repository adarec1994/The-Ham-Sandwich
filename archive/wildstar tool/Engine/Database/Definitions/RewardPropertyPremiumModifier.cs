namespace ProjectWS.Engine.Database.Definitions
{
	public class RewardPropertyPremiumModifier : TblRecord
	{
		public override string GetFileName() => "RewardPropertyPremiumModifier";
		public override uint GetID() => this.ID;
		
		public uint ID;
		public uint premiumSystemEnum;
		public uint tier;
		public uint rewardPropertyId;
		public uint rewardPropertyData;
		public uint modifierValueInt;
		public float modifierValueFloat;
		public uint entitlementIdModifierCount;
		public uint flags;
	}
}
