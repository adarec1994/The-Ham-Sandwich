namespace ProjectWS.Engine.Database.Definitions
{
	public class PathSoldierEvent : TblRecord
	{
		public override string GetFileName() => "PathSoldierEvent";
		public override uint GetID() => this.ID;
		
		public uint ID;
		public uint pathSoldierEventType;
		public uint maxTimeBetweenWaves;
		public uint maxEventTime;
		public uint towerDefenseAllowance;
		public uint towerDefenseBuildTimeMS;
		public uint initialSpawnTime;
	}
}
