namespace ProjectWS.Engine.Database.Definitions
{
	public class ArchiveEntry : TblRecord
	{
		public override string GetFileName() => "ArchiveEntry";
		public override uint GetID() => this.ID;
		
		public uint ID;
		public uint localizedTextIdHeading;
		public uint localizedTextIdText00;
		public uint localizedTextIdText01;
		public uint localizedTextIdText02;
		public uint localizedTextIdText03;
		public uint localizedTextIdText04;
		public uint localizedTextIdText05;
		public uint localizedTextIdTextScientist00;
		public uint localizedTextIdTextScientist01;
		public uint localizedTextIdTextScientist02;
		public uint localizedTextIdTextScientist03;
		public uint localizedTextIdTextScientist04;
		public uint localizedTextIdTextScientist05;
		public uint creature2IdPortrait;
		public string iconAssetPath;
		public string inlineAssetPath;
		public uint archiveEntryTypeEnum;
		public uint archiveEntryFlags;
		public uint archiveEntryHeaderEnum;
		public uint characterTitleIdReward;
	}
}
