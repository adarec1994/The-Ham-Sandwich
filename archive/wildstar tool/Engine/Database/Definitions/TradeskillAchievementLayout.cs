namespace ProjectWS.Engine.Database.Definitions
{
	public class TradeskillAchievementLayout : TblRecord
	{
		public override string GetFileName() => "TradeskillAchievementLayout";
		public override uint GetID() => this.ID;
		
		public uint ID;
		public uint achievementId;
		public uint achievementIdParent00;
		public uint achievementIdParent01;
		public uint achievementIdParent02;
		public uint achievementIdParent03;
		public uint achievementIdParent04;
		public uint gridX;
		public uint gridY;
	}
}
