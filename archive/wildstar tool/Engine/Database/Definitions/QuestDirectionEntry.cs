namespace ProjectWS.Engine.Database.Definitions
{
	public class QuestDirectionEntry : TblRecord
	{
		public override string GetFileName() => "QuestDirectionEntry";
		public override uint GetID() => this.ID;
		
		public uint ID;
		public uint worldLocation2Id;
		public uint worldLocation2IdInactive;
		public uint worldZoneId;
		public uint questDirectionEntryFlags;
		public uint questDirectionFactionEnum;
	}
}
