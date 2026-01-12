namespace ProjectWS.Engine.Database.Definitions
{
	public class ItemRuneSlotRandomization : TblRecord
	{
		public override string GetFileName() => "ItemRuneSlotRandomization";
		public override uint GetID() => this.ID;
		
		public uint ID;
		public uint microchipTypeEnum;
		public uint itemRoleFlagBitMask;
		public float randomWeight;
	}
}
