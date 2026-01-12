namespace ProjectWS.Engine.Database.Definitions
{
	public class ItemSlot : TblRecord
	{
		public override string GetFileName() => "ItemSlot";
		public override uint GetID() => this.ID;
		
		public uint ID;
		public string EnumName;
		public uint equippedSlotFlags;
		public float armorModifier;
		public float itemLevelModifier;
		public uint slotBonus;
		public uint glyphSlotBonus;
		public uint minLevel;
	}
}
