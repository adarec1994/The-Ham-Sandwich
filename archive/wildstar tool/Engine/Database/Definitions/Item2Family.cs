namespace ProjectWS.Engine.Database.Definitions
{
	public class Item2Family : TblRecord
	{
		public override string GetFileName() => "Item2Family";
		public override uint GetID() => this.ID;
		
		public uint ID;
		public uint localizedTextId;
		public uint flags;
		public uint soundEventIdEquip;
		public float vendorMultiplier;
		public float turninMultiplier;
	}
}
