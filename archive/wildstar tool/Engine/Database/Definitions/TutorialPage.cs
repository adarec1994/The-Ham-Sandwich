namespace ProjectWS.Engine.Database.Definitions
{
	public class TutorialPage : TblRecord
	{
		public override string GetFileName() => "TutorialPage";
		public override uint GetID() => this.ID;
		
		public uint ID;
		public uint tutorialId;
		public uint page;
		public uint tutorialLayoutId;
		public uint localizedTextIdTitle;
		public uint localizedTextIdBody00;
		public uint localizedTextIdBody01;
		public uint localizedTextIdBody02;
		public string sprite00;
		public string sprite01;
		public string sprite02;
		public uint soundEventId;
	}
}
