namespace ProjectWS.Engine.Database.Definitions
{
	public class ItemStat : TblRecord
	{
		public override string GetFileName() => "ItemStat";
		public override uint GetID() => this.ID;
		
		public uint ID;
		public uint itemStatTypeEnum00;
		public uint itemStatTypeEnum01;
		public uint itemStatTypeEnum02;
		public uint itemStatTypeEnum03;
		public uint itemStatTypeEnum04;
		public uint itemStatData00;
		public uint itemStatData01;
		public uint itemStatData02;
		public uint itemStatData03;
		public uint itemStatData04;
	}
}
