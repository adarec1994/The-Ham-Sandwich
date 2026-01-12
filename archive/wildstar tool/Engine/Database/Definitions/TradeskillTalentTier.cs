namespace ProjectWS.Engine.Database.Definitions
{
	public class TradeskillTalentTier : TblRecord
	{
		public override string GetFileName() => "TradeskillTalentTier";
		public override uint GetID() => this.ID;
		
		public uint ID;
		public uint tradeSkillId;
		public uint pointsToUnlock;
		public uint respecCost;
		public uint tradeSkillBonusId00;
		public uint tradeSkillBonusId01;
		public uint tradeSkillBonusId02;
		public uint tradeSkillBonusId03;
		public uint tradeSkillBonusId04;
	}
}
