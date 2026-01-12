namespace ProjectWS.Engine.Database.Definitions
{
	public class Tutorial : TblRecord
	{
		public override string GetFileName() => "Tutorial";
		public override uint GetID() => this.ID;
		
		public uint ID;
		public uint flags;
		public uint tutorialCategoryEnum;
		public uint localizedTextIdContextualPopup;
		public uint tutorialAnchorId;
		public uint requiredLevel;
		public uint prerequisiteId;
	}
}
