namespace ProjectWS.Engine.Database.Definitions
{
	public class PathSoldierAssassinate : TblRecord
	{
		public override string GetFileName() => "PathSoldierAssassinate";
		public override uint GetID() => this.ID;
		
		public uint ID;
		public uint creature2Id;
		public uint targetGroupId;
		public uint count;
	}
}
