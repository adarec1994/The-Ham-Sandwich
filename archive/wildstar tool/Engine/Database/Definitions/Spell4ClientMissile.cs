namespace ProjectWS.Engine.Database.Definitions
{
	public class Spell4ClientMissile : TblRecord
	{
		public override string GetFileName() => "Spell4ClientMissile";
		public override uint GetID() => this.ID;
		
		public uint ID;
		public uint missileType;
		public uint castStage;
		public uint originUnitEnum;
		public uint targetFlags;
		public string modelPath;
		public string fxPath;
		public string beamPath;
		public uint beamSource;
		public uint beamTarget;
		public uint itemSlot;
		public uint costumeSide;
		public uint modelAttachmentIdCaster;
		public uint modelAttachmentIdTarget;
		public uint clientDelay;
		public uint modelEventIdDelayedBy;
		public uint flags;
		public uint duration;
		public uint frequency;
		public uint speedMps;
		public float accMpss;
		public uint revolverNestedMissileInitDelay;
		public uint revolverNestedMissileSubDelay;
		public uint spell4ClientMissileIdNested;
		public string revolverMissileImpactAssetPath;
		public uint missileRevolverTrackId;
		public string birthAnchorPath;
		public string deathAnchorPath;
		public string trajAnchorPath;
		public float birthDuration;
		public float birthAnchorAngleMin;
		public float birthAnchorAngleMax;
		public float deathAnchorAngleMin;
		public float deathAnchorAngleMax;
		public uint deathAnchorSpace;
		public uint itemSlotIdObj;
		public uint objCostumeSide;
		public float trajPoseFullBlendDistance;
		public float trajAnchorPlaySpeed;
		public float parabolaHeightScale;
		public float rotateX;
		public float rotateY;
		public float rotateZ;
		public float scale;
		public float endScale;
		public uint phaseFlags;
		public uint telegraphDamageIdAttach;
		public uint soundEventIdBirth;
		public uint soundEventIdLoopStart;
		public uint soundEventIdLoopStop;
		public uint soundEventIdDeath;
		public uint beamDiffuseColor;
		public uint missileDiffuseColor;
	}
}
