namespace ProjectWS.Engine.Database.Definitions
{
	public class PathSoldierTowerDefense : TblRecord
	{
		public override string GetFileName() => "PathSoldierTowerDefense";
		public override uint GetID() => this.ID;
		
		public uint ID;
		public uint pathSoldierEventId;
		public uint buildCost;
		public uint localizedTextIdName;
		public uint worldLocation2IdDisplay;
		public uint towerDefenseBuildType;
		public uint spell4Id;
		public uint soldierTowerDefenseFlags;
		public uint soldierTowerDefenseImprovementType;
	}
}
