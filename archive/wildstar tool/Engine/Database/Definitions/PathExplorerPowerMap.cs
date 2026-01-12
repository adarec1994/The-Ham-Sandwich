namespace ProjectWS.Engine.Database.Definitions
{
	public class PathExplorerPowerMap : TblRecord
	{
		public override string GetFileName() => "PathExplorerPowerMap";
		public override uint GetID() => this.ID;
		
		public uint ID;
		public uint distanceThreshold;
		public uint collectQuantity;
		public uint victoryPauseMS;
		public uint worldLocation2IdVisual;
		public uint visualEffectIdInactive;
		public uint localizedTextIdInfo;
	}
}
