namespace ProjectWS.Engine.Database.Definitions
{
	public class PeriodicQuestSetCategory : TblRecord
	{
		public override string GetFileName() => "PeriodicQuestSetCategory";
		public override uint GetID() => this.ID;
		
		public uint ID;
		public uint periodicSetsOffered;
	}
}
