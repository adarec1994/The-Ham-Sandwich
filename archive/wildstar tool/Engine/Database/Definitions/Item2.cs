namespace ProjectWS.Engine.Database.Definitions
{
	public class Item2 : TblRecord
	{
		public override string GetFileName() => "Item2";
		public override uint GetID() => this.ID;
		
		public uint ID;
		public uint itemBudgetId;
		public uint itemStatId;
		public uint itemRuneInstanceId;
		public uint itemQualityId;
		public uint itemSpecialId00;
		public uint itemImbuementId;
		public uint item2FamilyId;
		public uint item2CategoryId;
		public uint item2TypeId;
		public uint itemDisplayId;
		public uint itemSourceId;
		public uint classRequired;
		public uint raceRequired;
		public uint faction2IdRequired;
		public uint powerLevel;
		public uint requiredLevel;
		public uint requiredItemLevel;
		public uint prerequisiteId;
		public uint equippedSlotFlags;
		public uint maxStackCount;
		public uint maxCharges;
		public uint expirationTimeMinutes;
		public uint quest2IdActivation;
		public uint quest2IdActivationRequired;
		public uint questObjectiveActivationRequired;
		public uint tradeskillAdditiveId;
		public uint tradeskillCatalystId;
		public uint housingDecorInfoId;
		public uint housingWarplotBossTokenId;
		public uint genericUnlockSetId;
		public uint flags;
		public uint bindFlags;
		public uint buyFromVendorStackCount;
		public uint currencyTypeId0;
		public uint currencyTypeId1;
		public uint currencyAmount0;
		public uint currencyAmount1;
		public uint currencyTypeId0SellToVendor;
		public uint currencyTypeId1SellToVendor;
		public uint currencyAmount0SellToVendor;
		public uint currencyAmount1SellToVendor;
		public uint itemColorSetId;
		public float supportPowerPercentage;
		public uint localizedTextIdName;
		public uint localizedTextIdTooltip;
		public string buttonTemplate;
		public string buttonIcon;
		public uint soundEventIdEquip;
	}
}
