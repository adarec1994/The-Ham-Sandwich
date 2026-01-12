namespace ProjectWS.Engine.Database.Definitions
{
	public class PathScientistExperimentation : TblRecord
	{
		public override string GetFileName() => "PathScientistExperimentation";
		public override uint GetID() => this.ID;
		
		public uint ID;
		public uint numAttempts;
	}
}
