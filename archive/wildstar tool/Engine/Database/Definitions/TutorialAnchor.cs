namespace ProjectWS.Engine.Database.Definitions
{
	public class TutorialAnchor : TblRecord
	{
		public override string GetFileName() => "TutorialAnchor";
		public override uint GetID() => this.ID;
		
		public uint ID;
		public uint tutorialAnchorOrientationEnum;
		public uint hOffset;
		public uint vOffset;
		public uint flags;
	}
}
