namespace ProjectWS.Engine.Database.Definitions
{
	public class PathSoldierActivate : TblRecord
	{
		public override string GetFileName() => "PathSoldierActivate";
		public override uint GetID() => this.ID;
		
		public uint ID;
		public uint creature2Id;
		public uint targetGroupId;
		public uint count;
		public uint soldierActivateModeEnum;
	}
}
