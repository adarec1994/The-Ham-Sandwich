namespace ProjectWS.Engine.Database.Definitions
{
	public class PathExplorerArea : TblRecord
	{
		public override string GetFileName() => "PathExplorerArea";
		public override uint GetID() => this.ID;
		
		public uint ID;
		public uint pathSettlerHubId;
	}
}
