namespace ProjectWS.Engine.Database.Definitions
{
	public class ZoneCompletion : TblRecord
	{
		public override string GetFileName() => "ZoneCompletion";
		public override uint GetID() => this.ID;
		
		public uint ID;
		public uint mapZoneId;
		public uint zoneCompletionFactionEnum;
		public uint episodeQuestCount;
		public uint taskQuestCount;
		public uint challengeCount;
		public uint datacubeCount;
		public uint taleCount;
		public uint journalCount;
		public uint characterTitleIdReward;
	}
}
