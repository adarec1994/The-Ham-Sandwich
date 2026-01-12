namespace ProjectWS.Engine.Database.Definitions
{
	public class QuestDirection : TblRecord
	{
		public override string GetFileName() => "QuestDirection";
		public override uint GetID() => this.ID;
		
		public uint ID;
		public uint questDirectionFlags;
		public uint questDirectionEntryId00;
		public uint questDirectionEntryId01;
		public uint questDirectionEntryId02;
		public uint questDirectionEntryId03;
		public uint questDirectionEntryId04;
		public uint questDirectionEntryId05;
		public uint questDirectionEntryId06;
		public uint questDirectionEntryId07;
		public uint questDirectionEntryId08;
		public uint questDirectionEntryId09;
		public uint questDirectionEntryId10;
		public uint questDirectionEntryId11;
		public uint questDirectionEntryId12;
		public uint questDirectionEntryId13;
		public uint questDirectionEntryId14;
		public uint questDirectionEntryId15;
		public uint worldZoneIdExcludedZone;
	}
}
