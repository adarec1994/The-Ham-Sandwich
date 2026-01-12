namespace ProjectWS.Engine.Database.Definitions
{
	public class CurrencyType : TblRecord
	{
		public override string GetFileName() => "CurrencyType";
		public override uint GetID() => this.ID;
		
		public uint ID;
		public string description;
		public uint localizedTextId;
		public string iconName;
		public ulong capAmount;
	}
}
