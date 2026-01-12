namespace ProjectWS.Engine.Database.Definitions
{
	public class PathSoldierSWAT : TblRecord
	{
		public override string GetFileName() => "PathSoldierSWAT";
		public override uint GetID() => this.ID;
		
		public uint ID;
		public uint creature2Id;
		public uint targetGroupId;
		public uint count;
		public uint virtualItemIdDisplay;
	}
}
