namespace ProjectWS.Engine.Database.Definitions
{
	public class ItemRandomStat : TblRecord
	{
		public override string GetFileName() => "ItemRandomStat";
		public override uint GetID() => this.ID;
		
		public uint ID;
		public uint itemRandomStatGroupId;
		public float weight;
		public uint itemStatTypeEnum;
		public uint itemStatData;
	}
}
