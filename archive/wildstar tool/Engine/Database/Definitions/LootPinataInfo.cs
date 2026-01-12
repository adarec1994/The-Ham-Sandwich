namespace ProjectWS.Engine.Database.Definitions
{
	public class LootPinataInfo : TblRecord
	{
		public override string GetFileName() => "LootPinataInfo";
		public override uint GetID() => this.ID;
		
		public uint ID;
		public uint item2Id;
		public uint item2TypeId;
		public uint item2CategoryId;
		public uint item2FamilyId;
		public uint virtualItemId;
		public uint accountCurrencyTypeId;
		public uint creature2IdChest;
		public float mass;
		public uint soundEventId;
	}
}
