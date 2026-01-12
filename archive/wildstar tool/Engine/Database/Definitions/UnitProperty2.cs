namespace ProjectWS.Engine.Database.Definitions
{
	public class UnitProperty2 : TblRecord
	{
		public override string GetFileName() => "UnitProperty2";
		public override uint GetID() => this.ID;
		
		public uint ID;
		public string description;
		public string enumName;
		public float defaultValue;
		public uint localizedTextId;
		public float valuePerPoint;
		public uint flags;
		public uint tooltipDisplayOrder;
		public uint profiencyFlagEnum;
		public uint itemCraftingGroupFlagBitMask;
		public uint equippedSlotFlags;
	}
}
