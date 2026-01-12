namespace ProjectWS.Engine.Database.Definitions
{
	public class TradeskillAchievementReward : TblRecord
	{
		public override string GetFileName() => "TradeskillAchievementReward";
		public override uint GetID() => this.ID;
		
		public uint ID;
		public uint achievementId;
		public uint faction2Id;
		public uint factionIdAmount;
		public uint talentPoints;
		public uint tradeSkillSchematicId00;
		public uint tradeSkillSchematicId01;
		public uint tradeSkillSchematicId02;
		public uint tradeSkillSchematicId03;
		public uint tradeSkillSchematicId04;
		public uint tradeSkillSchematicId05;
		public uint tradeSkillSchematicId06;
		public uint tradeSkillSchematicId07;
	}
}
