namespace ProjectWS.Engine.Database.Definitions
{
	public class Spell4Visual : TblRecord
	{
		public override string GetFileName() => "Spell4Visual";
		public override uint GetID() => this.ID;
		
		public uint ID;
		public uint targetTypeFlags;
		public uint stageType;
		public uint stageFlags;
		public uint visualEffectId;
		public uint visualEffectIdSound;
		public uint modelEventIdDelay;
		public uint soundEventType00;
		public uint soundEventType01;
		public uint soundEventType02;
		public uint soundEventType03;
		public uint soundEventType04;
		public uint soundEventType05;
		public uint soundImpactDescriptionIdTarget00;
		public uint soundImpactDescriptionIdTarget01;
		public uint soundImpactDescriptionIdTarget02;
		public uint soundImpactDescriptionIdTarget03;
		public uint soundImpactDescriptionIdTarget04;
		public uint soundImpactDescriptionIdTarget05;
		public uint soundImpactDescriptionIdOrigin00;
		public uint soundImpactDescriptionIdOrigin01;
		public uint soundImpactDescriptionIdOrigin02;
		public uint soundImpactDescriptionIdOrigin03;
		public uint soundImpactDescriptionIdOrigin04;
		public uint soundImpactDescriptionIdOrigin05;
		public uint modelAttachmentIdCaster;
		public uint phaseFlags;
		public float modelOffsetX;
		public float modelOffsetY;
		public float modelOffsetZ;
		public float modelScale;
		public uint preDelayTimeMs;
		public uint telegraphDamageIdAttach;
		public uint prerequisiteId;
	}
}
