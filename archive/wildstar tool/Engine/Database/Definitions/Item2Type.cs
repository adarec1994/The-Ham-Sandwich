namespace ProjectWS.Engine.Database.Definitions
{
	public class Item2Type : TblRecord
	{
		public override string GetFileName() => "Item2Type";
		public override uint GetID() => this.ID;
		
		public uint ID;
		public uint localizedTextId;
		public uint itemSlotId;
		public uint soundEventIdLoot;
		public uint soundEventIdEquip;
		public uint flags;
		public float vendorMultiplier;
		public float turninMultiplier;
		public uint Item2CategoryId;
		public float referenceMuzzleX;
		public float referenceMuzzleY;
		public float referenceMuzzleZ;
	}
}
