namespace ProjectWS.Engine.Database.Definitions
{
	public class PathExplorerDoor : TblRecord
	{
		public override string GetFileName() => "PathExplorerDoor";
		public override uint GetID() => this.ID;
		
		public uint ID;
		public uint worldZoneIdInsideMicro;
		public uint targetGroupIdActivate;
		public uint targetGroupIdKill;
	}
}
