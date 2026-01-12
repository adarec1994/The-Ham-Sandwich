namespace ProjectWS.Engine.Database.Definitions
{
	public class ArchiveArticle : TblRecord
	{
		public override string GetFileName() => "ArchiveArticle";
		public override uint GetID() => this.ID;
		
		public uint ID;
		public uint creature2IdPortrait;
		public string iconAssetPath;
		public uint localizedTextIdTitle;
		public uint localizedTextIdText00;
		public uint localizedTextIdText01;
		public uint localizedTextIdText02;
		public uint localizedTextIdText03;
		public uint localizedTextIdText04;
		public uint localizedTextIdText05;
		public uint archiveEntryId00;
		public uint archiveEntryId01;
		public uint archiveEntryId02;
		public uint archiveEntryId03;
		public uint archiveEntryId04;
		public uint archiveEntryId05;
		public uint archiveEntryId06;
		public uint archiveEntryId07;
		public uint archiveEntryId08;
		public uint archiveEntryId09;
		public uint archiveEntryId10;
		public uint archiveEntryId11;
		public uint archiveEntryId12;
		public uint archiveEntryId13;
		public uint archiveEntryId14;
		public uint archiveEntryId15;
		public uint archiveArticleFlags;
		public uint archiveCategoryId00;
		public uint archiveCategoryId01;
		public uint archiveCategoryId02;
		public uint localizedTextIdToolTip;
		public uint worldZoneId;
		public uint characterTitleIdReward;
		public string linkName;
	}
}
