namespace ProjectWS.Engine.Database.Definitions
{
	public class AchievementText : TblRecord
	{
		public override string GetFileName() => "AchievementText";
		public override uint GetID() => this.ID;
		
		public uint ID;
		public uint localizedTextId;
	}
}
