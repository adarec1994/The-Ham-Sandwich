namespace ProjectWS.Engine.Database.Definitions
{
	public class TradeskillProficiency : TblRecord
	{
		public override string GetFileName() => "TradeskillProficiency";
		public override uint GetID() => this.ID;
		
		public uint ID;
		public uint proficiencyFlagEnum;
	}
}
