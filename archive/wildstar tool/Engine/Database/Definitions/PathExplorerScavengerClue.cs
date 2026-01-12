namespace ProjectWS.Engine.Database.Definitions
{
	public class PathExplorerScavengerClue : TblRecord
	{
		public override string GetFileName() => "PathExplorerScavengerClue";
		public override uint GetID() => this.ID;
		
		public uint ID;
		public uint localizedTextIdClue;
		public uint explorerScavengerClueTypeEnum;
		public uint creature2Id;
		public uint targetGroupId;
		public float activeRadius;
		public uint worldLocation2IdMiniMap;
	}
}
