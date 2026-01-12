namespace ProjectWS.Engine.Database.Definitions
{
	public class TradeskillCatalystOrdering : TblRecord
	{
		public override string GetFileName() => "TradeskillCatalystOrdering";
		public override uint GetID() => this.ID;
		
		public uint ID;
		public uint unlockLevel00;
		public uint unlockLevel01;
		public uint unlockLevel02;
		public uint unlockLevel03;
		public uint unlockLevel04;
	}
}
