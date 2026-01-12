namespace ProjectWS.Engine.Database.Definitions
{
	public class PeriodicQuestGroup : TblRecord
	{
		public override string GetFileName() => "PeriodicQuestGroup";
		public override uint GetID() => this.ID;
		
		public uint ID;
		public uint periodicQuestSetId;
		public uint periodicQuestsOffered;
		public uint maxPeriodicQuestsAllowed;
		public uint weight;
		public uint contractTypeEnum;
		public uint contractQualityEnum;
	}
}
