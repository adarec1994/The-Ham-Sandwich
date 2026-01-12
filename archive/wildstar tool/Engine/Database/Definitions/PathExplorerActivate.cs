namespace ProjectWS.Engine.Database.Definitions
{
	public class PathExplorerActivate : TblRecord
	{
		public override string GetFileName() => "PathExplorerActivate";
		public override uint GetID() => this.ID;
		
		public uint ID;
		public uint creature2Id;
		public uint targetGroupId;
		public uint count;
	}
}
