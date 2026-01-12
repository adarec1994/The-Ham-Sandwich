namespace ProjectWS.Engine.Database.Definitions
{
	public class TutorialLayout : TblRecord
	{
		public override string GetFileName() => "TutorialLayout";
		public override uint GetID() => this.ID;
		
		public uint ID;
		public string form;
		public uint flags;
	}
}
