namespace ProjectWS.Engine.Database.Definitions
{
	public class PathExplorerNode : TblRecord
	{
		public override string GetFileName() => "PathExplorerNode";
		public override uint GetID() => this.ID;
		
		public uint ID;
		public uint pathExplorerAreaId;
		public uint worldLocation2Id;
		public uint spline2Id;
		public uint localizedTextIdSettlerButton;
		public uint questDirectionId;
		public uint visualEffectId;
	}
}
