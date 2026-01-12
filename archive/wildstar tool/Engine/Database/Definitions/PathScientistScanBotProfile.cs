namespace ProjectWS.Engine.Database.Definitions
{
	public class PathScientistScanBotProfile : TblRecord
	{
		public override string GetFileName() => "PathScientistScanBotProfile";
		public override uint GetID() => this.ID;
		
		public uint ID;
		public uint localizedTextId;
		public uint creature2Id;
		public uint scanTimeMS;
		public uint processingTimeMS;
		public float playerRadius;
		public float scanAOERange;
		public float maxSeekDistance;
		public float speedMultiplier;
		public float thoroughnessMultiplier;
		public float healthMultiplier;
		public float healthRegenMultiplier;
		public uint minCooldownTimeMs;
		public uint maxCooldownTimeMs;
		public float maxCooldownDistance;
		public uint pathScientistScanBotProfileFlags;
		public uint socketCount;
	}
}
