namespace ProjectWS.Engine.Database.Definitions
{
	public class PathSettlerInfrastructure : TblRecord
	{
		public override string GetFileName() => "PathSettlerInfrastructure";
		public override uint GetID() => this.ID;
		
		public uint ID;
		public uint pathSettlerHubId00;
		public uint pathSettlerHubId01;
		public uint localizedTextIdObjective;
		public uint missionCount;
		public uint worldZoneId;
		public uint count;
		public uint maxTime;
		public uint creature2IdDepot;
		public uint creature2IdResource00;
		public uint creature2IdResource01;
		public uint creature2IdResource02;
		public uint spell4IdResource00;
		public uint spell4IdResource01;
		public uint spell4IdResource02;
		public uint maxStackCountResource00;
		public uint maxStackCountResource01;
		public uint maxStackCountResource02;
	}
}
