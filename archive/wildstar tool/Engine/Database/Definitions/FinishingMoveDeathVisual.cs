namespace ProjectWS.Engine.Database.Definitions
{
	public class FinishingMoveDeathVisual : TblRecord
	{
		public override string GetFileName() => "FinishingMoveDeathVisual";
		public override uint GetID() => this.ID;
		
		public uint ID;
		public uint priority;
		public uint damageTypeFlags;
		public uint creature2MinSize;
		public uint creature2MaxSize;
		public uint creatureMaterialEnum;
		public uint movementStateFlags;
		public string deathModelAsset;
		public uint modelSequenceIdDeath;
		public uint visualEffectIdDeath00;
		public uint visualEffectIdDeath01;
		public uint visualEffectIdDeath02;
	}
}
