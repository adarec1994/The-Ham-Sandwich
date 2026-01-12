namespace ProjectWS.Engine.Database.Definitions
{
	public class PathScientistDatacubeDiscovery : TblRecord
	{
		public override string GetFileName() => "PathScientistDatacubeDiscovery";
		public override uint GetID() => this.ID;
		
		public uint ID;
		public uint worldZoneId;
	}
}
