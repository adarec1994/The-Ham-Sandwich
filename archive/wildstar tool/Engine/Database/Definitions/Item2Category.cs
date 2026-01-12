namespace ProjectWS.Engine.Database.Definitions
{
	public class Item2Category : TblRecord
	{
		public override string GetFileName() => "Item2Category";
		public override uint GetID() => this.ID;
		
		public uint ID;
		public uint localizedTextId;
		public uint itemProficiencyId;
		public uint flags;
		public uint tradeSkillId;
		public uint soundEventIdEquip;
		public float vendorMultiplier;
		public float turninMultiplier;
		public float armorModifier;
		public float armorBase;
		public float weaponPowerModifier;
		public uint weaponPowerBase;
		public uint item2FamilyId;
	}
}
