namespace ProjectWS.Engine.Database.Definitions
{
	public class TradeskillCatalyst : TblRecord
	{
		public override string GetFileName() => "TradeskillCatalyst";
		public override uint GetID() => this.ID;
		
		public uint ID;
		public uint tradeSkillId;
		public uint tier;
		public uint tradeskillCatalystEnum00;
		public uint tradeskillCatalystEnum01;
		public uint tradeskillCatalystEnum02;
		public uint tradeskillCatalystEnum03;
		public uint tradeskillCatalystEnum04;
		public float value00;
		public float value01;
		public float value02;
		public float value03;
		public float value04;
	}
}
