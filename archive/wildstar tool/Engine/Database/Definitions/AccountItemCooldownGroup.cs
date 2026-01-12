namespace ProjectWS.Engine.Database.Definitions
{
	public class AccountItemCooldownGroup : TblRecord
	{
		public override string GetFileName() => "AccountItemCooldownGroup";
		public override uint GetID() => this.ID;
		
		public uint ID;
	}
}
