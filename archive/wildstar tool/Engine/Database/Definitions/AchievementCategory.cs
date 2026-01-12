namespace ProjectWS.Engine.Database.Definitions
{
	public class AchievementCategory : TblRecord
	{
		public override string GetFileName() => "AchievementCategory";
		public override uint GetID() => this.ID;
		
		public uint ID;
		public uint localizedTextId;
		public uint localizedTextIdFullName;
		public uint achievementCategoryIdParent;
	}
}
