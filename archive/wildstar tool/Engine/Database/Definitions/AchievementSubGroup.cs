namespace ProjectWS.Engine.Database.Definitions
{
	public class AchievementSubGroup : TblRecord
	{
		public override string GetFileName() => "AchievementSubGroup";
		public override uint GetID() => this.ID;
		
		public uint ID;
		public uint localizedTextId;
		public uint tier;
	}
}
