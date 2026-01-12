namespace ProjectWS.Engine.Database.Definitions
{
	public class QuestCategory : TblRecord
	{
		public override string GetFileName() => "QuestCategory";
		public override uint GetID() => this.ID;
		
		public uint ID;
		public string description;
		public uint localizedTextIdTitle;
		public uint questCategoryTypeEnum;
	}
}
