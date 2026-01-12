namespace ProjectWS.Engine.Database.Definitions
{
	public class ArchiveEntryUnlockRule : TblRecord
	{
		public override string GetFileName() => "ArchiveEntryUnlockRule";
		public override uint GetID() => this.ID;
		
		public uint ID;
		public uint archiveEntryId;
		public uint archiveEntryUnlockRuleEnum;
		public uint archiveEntryUnlockRuleFlags;
		public uint object00;
		public uint object01;
		public uint object02;
		public uint object03;
		public uint object04;
		public uint object05;
	}
}
