namespace ProjectWS.Engine.Database.Definitions
{
	public class StoreKeyword : TblRecord
	{
		public override string GetFileName() => "StoreKeyword";
		public override uint GetID() => this.ID;
		
		public uint ID;
		public uint storeDisplayInfoId;
		public string keyword;
	}
}
