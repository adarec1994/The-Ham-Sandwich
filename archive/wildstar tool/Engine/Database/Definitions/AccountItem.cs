namespace ProjectWS.Engine.Database.Definitions
{
	public class AccountItem : TblRecord
	{
		public override string GetFileName() => "AccountItem";
		public override uint GetID() => this.ID;
		
		public uint ID;
		public uint flags;
		public uint item2Id;
		public uint entitlementId;
		public uint entitlementCount;
		public uint entitlementScopeEnum;
		public uint instantEventEnum;
		public uint accountCurrencyEnum;
		public ulong accountCurrencyAmount;
		public string buttonIcon;
		public uint prerequisiteId;
		public uint accountItemCooldownGroupId;
		public uint storeDisplayInfoId;
		public string storeIdentifierUpsell;
		public uint creature2DisplayGroupIdGacha;
		public uint entitlementIdPurchase;
		public uint genericUnlockSetId;
	}
}
