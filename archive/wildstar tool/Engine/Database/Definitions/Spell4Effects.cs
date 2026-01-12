namespace ProjectWS.Engine.Database.Definitions
{
	public class Spell4Effects : TblRecord
	{
		public override string GetFileName() => "Spell4Effects";
		public override uint GetID() => this.ID;
		
		public uint ID;
		public uint spellId;
		public uint targetFlags;
		public uint effectType;
		public uint damageType;
		public uint delayTime;
		public uint tickTime;
		public uint durationTime;
		public uint flags;
		public uint dataBits00;
		public uint dataBits01;
		public uint dataBits02;
		public uint dataBits03;
		public uint dataBits04;
		public uint dataBits05;
		public uint dataBits06;
		public uint dataBits07;
		public uint dataBits08;
		public uint dataBits09;
		public uint innateCostPerTickType0;
		public uint innateCostPerTickType1;
		public uint innateCostPerTick0;
		public uint innateCostPerTick1;
		public uint emmComparison;
		public uint emmValue;
		public float threatMultiplier;
		public uint spell4EffectGroupListId;
		public uint prerequisiteIdCasterApply;
		public uint prerequisiteIdTargetApply;
		public uint prerequisiteIdCasterPersistence;
		public uint prerequisiteIdTargetPersistence;
		public uint prerequisiteIdTargetSuspend;
		public uint parameterType00;
		public uint parameterType01;
		public uint parameterType02;
		public uint parameterType03;
		public float parameterValue00;
		public float parameterValue01;
		public float parameterValue02;
		public float parameterValue03;
		public uint phaseFlags;
		public uint orderIndex;
	}
}
