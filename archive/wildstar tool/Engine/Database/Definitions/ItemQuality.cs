namespace ProjectWS.Engine.Database.Definitions
{
	public class ItemQuality : TblRecord
	{
		public override string GetFileName() => "ItemQuality";
		public override uint GetID() => this.ID;
		
		public uint ID;
		public float salvageCritChance;
		public float turninMultiplier;
		public float runeCostMultiplier;
		public float dyeCostMultiplier;
		public uint visualEffectIdLoot;
		public float iconColorR;
		public float iconColorG;
		public float iconColorB;
		public uint defaultRunes;
		public uint maxRunes;
		public string assetPathDieModel;
		public uint soundEventIdFortuneCardFanfare;
	}
}
