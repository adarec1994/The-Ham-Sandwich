namespace ProjectWS.Engine.Database.Definitions
{
	public class PathSoldierEventWave : TblRecord
	{
		public override string GetFileName() => "PathSoldierEventWave";
		public override uint GetID() => this.ID;
		
		public uint ID;
		public uint pathSoldierEventId;
		public uint waveIndex;
		public uint soundZoneKitId;
	}
}
