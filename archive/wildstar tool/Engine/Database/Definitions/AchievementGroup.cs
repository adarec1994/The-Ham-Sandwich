namespace ProjectWS.Engine.Database.Definitions
{
	public class AchievementGroup : TblRecord
	{
		public override string GetFileName() => "AchievementGroup";
		public override uint GetID() => this.ID;
		
		public uint ID;
		public uint localizedTextId;
		public uint tradeSkillId;
	}
}
