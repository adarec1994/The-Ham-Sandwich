namespace ProjectWS.Engine.Database.Definitions
{
	public class PeriodicQuestSet : TblRecord
	{
		public override string GetFileName() => "PeriodicQuestSet";
		public override uint GetID() => this.ID;
		
		public uint ID;
		public uint periodicQuestSetCategoryId;
		public uint periodicGroupsOffered;
		public uint weight;
	}
}
