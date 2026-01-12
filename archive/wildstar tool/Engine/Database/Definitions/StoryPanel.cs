namespace ProjectWS.Engine.Database.Definitions
{
	public class StoryPanel : TblRecord
	{
		public override string GetFileName() => "StoryPanel";
		public override uint GetID() => this.ID;
		
		public uint ID;
		public uint localizedTextIdBody;
		public uint soundEventId;
		public uint windowTypeId;
		public uint durationMS;
		public uint prerequisiteId;
		public uint storyPanelStyleEnum;
	}
}
