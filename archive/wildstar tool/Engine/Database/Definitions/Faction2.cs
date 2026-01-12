namespace ProjectWS.Engine.Database.Definitions
{
	public class Faction2 : TblRecord
	{
		public override string GetFileName() => "Faction2";
		public override uint GetID() => this.ID;
		
		public uint ID;
		public uint faction2IdParent;
		public uint flags;
		public uint localizedTextIdName;
		public uint localizedTextIdToolTip;
		public uint orderIndex;
		public uint archiveArticleId;
	}
}
