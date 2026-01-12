namespace ProjectWS.Engine.Database.Definitions
{
	public class TelegraphDamage : TblRecord
	{
		public override string GetFileName() => "TelegraphDamage";
		public override uint GetID() => this.ID;
		
		public uint ID;
		public uint telegraphSubtypeEnum;
		public uint damageShapeEnum;
		public float param00;
		public float param01;
		public float param02;
		public float param03;
		public float param04;
		public float param05;
		public uint telegraphTimeStartMs;
		public uint telegraphTimeEndMs;
		public uint telegraphTimeRampInMs;
		public uint telegraphTimeRampOutMs;
		public float xPositionOffset;
		public float yPositionOffset;
		public float zPositionOffset;
		public float rotationDegrees;
		public uint telegraphDamageFlags;
		public uint targetTypeFlags;
		public uint phaseFlags;
		public uint prerequisiteIdCaster;
		public uint spellThresholdRestrictionFlags;
		public uint displayFlags;
		public uint opacityModifier;
		public uint displayGroup;
	}
}
