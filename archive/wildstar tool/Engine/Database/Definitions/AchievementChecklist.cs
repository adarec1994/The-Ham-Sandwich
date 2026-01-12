namespace ProjectWS.Engine.Database.Definitions
{
	public class AchievementChecklist : TblRecord
	{
		public override string GetFileName() => "AchievementChecklist";
		public override uint GetID() => this.ID;
		
		public uint ID;
		public uint achievementId;
		public uint bit;
		public uint objectId;
		public uint objectIdAlt;
		public uint prerequisiteId;
		public uint prerequisiteIdAlt;
	}
}
