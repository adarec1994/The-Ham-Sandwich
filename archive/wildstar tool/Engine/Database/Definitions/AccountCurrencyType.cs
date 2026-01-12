namespace ProjectWS.Engine.Database.Definitions
{
	public class AccountCurrencyType : TblRecord
	{
		public override string GetFileName() => "AccountCurrencyType";
		public override uint GetID() => this.ID;
		
		public uint ID;
		public uint localizedTextId;
		public string iconName;
		public uint accountItemId;
	}
}
