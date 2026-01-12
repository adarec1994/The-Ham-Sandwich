namespace ProjectWS.Engine.Database.Definitions
{
	public class PathExplorerScavengerHunt : TblRecord
	{
		public override string GetFileName() => "PathExplorerScavengerHunt";
		public override uint GetID() => this.ID;
		
		public uint ID;
		public uint creature2IdStart;
		public uint pathExplorerScavengerClueId00;
		public uint pathExplorerScavengerClueId01;
		public uint pathExplorerScavengerClueId02;
		public uint pathExplorerScavengerClueId03;
		public uint pathExplorerScavengerClueId04;
		public uint pathExplorerScavengerClueId05;
		public uint pathExplorerScavengerClueId06;
		public uint localizedTextIdStart;
		public uint spell4IdRelic;
		public string assetPathSprite;
	}
}
