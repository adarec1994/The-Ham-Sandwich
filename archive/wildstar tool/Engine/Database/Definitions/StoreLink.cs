namespace ProjectWS.Engine.Database.Definitions
{
	public class StoreLink : TblRecord
	{
		public override string GetFileName() => "StoreLink";
		public override uint GetID() => this.ID;
		
		public uint ID;
		public string enumName;
		public uint categoryData;
		public uint categoryDataPTR;
		public uint accountItemId;
	}
}
