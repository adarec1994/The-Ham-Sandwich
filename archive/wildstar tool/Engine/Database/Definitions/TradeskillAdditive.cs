namespace ProjectWS.Engine.Database.Definitions
{
	public class TradeskillAdditive : TblRecord
	{
		public override string GetFileName() => "TradeskillAdditive";
		public override uint GetID() => this.ID;
		
		public uint ID;
		public uint tradeSkillId;
		public uint tier;
		public float vectorX;
		public float vectorY;
		public float radius;
	}
}
