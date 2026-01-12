namespace ProjectWS.Engine.Database.Definitions
{
	public class ArchiveLink : TblRecord
	{
		public override string GetFileName() => "ArchiveLink";
		public override uint GetID() => this.ID;
		
		public uint ID;
		public uint archiveArticleIdParent;
		public uint archiveArticleIdChild;
		public uint archiveLinkFlags;
	}
}
