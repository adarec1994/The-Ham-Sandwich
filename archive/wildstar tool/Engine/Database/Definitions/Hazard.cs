namespace ProjectWS.Engine.Database.Definitions
{
	public class Hazard : TblRecord
	{
		public override string GetFileName() => "Hazard";
		public override uint GetID() => this.ID;
		
		public uint ID;
		public uint localizedTextIdName;
		public uint localizedTextIdTooltip;
		public float meterChangeRate;
		public uint meterMaxValue;
		public uint flags;
		public uint hazardTypeEnum;
		public uint spell4IdDamage;
		public float minDistanceToUnit;
		public float meterThreshold00;
		public float meterThreshold01;
		public float meterThreshold02;
		public uint spell4IdThresholdProc00;
		public uint spell4IdThresholdProc01;
		public uint spell4IdThresholdProc02;
	}
}
