namespace ProjectWS.Engine.Database.Definitions
{
	public class Spell4Thresholds : TblRecord
	{
		public override string GetFileName() => "Spell4Thresholds";
		public override uint GetID() => this.ID;
		
		public uint ID;
		public uint spell4IdParent;
		public uint spell4IdToCast;
		public uint orderIndex;
		public uint thresholdDuration;
		public uint vitalEnumCostType00;
		public uint vitalEnumCostType01;
		public uint vitalCostValue00;
		public uint vitalCostValue01;
		public uint localizedTextIdTooltip;
		public string iconReplacement;
		public uint visualEffectId;
	}
}
