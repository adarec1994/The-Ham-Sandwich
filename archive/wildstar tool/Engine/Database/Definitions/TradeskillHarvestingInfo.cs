namespace ProjectWS.Engine.Database.Definitions
{
	public class TradeskillHarvestingInfo : TblRecord
	{
		public override string GetFileName() => "TradeskillHarvestingInfo";
		public override uint GetID() => this.ID;
		
		public uint ID;
		public uint tradeSkillTierId;
		public uint prerequisiteId;
		public uint miniMapMarkerId;
	}
}
