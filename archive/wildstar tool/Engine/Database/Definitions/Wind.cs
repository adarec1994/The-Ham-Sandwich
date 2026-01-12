namespace ProjectWS.Engine.Database.Definitions
{
	public class Wind : TblRecord
	{
		public override string GetFileName() => "Wind";
		public override uint GetID() => this.ID;
		
		public uint ID;
		public uint type;
		public uint duration;
		public float radiusEnd;
		public float direction;
		public float directionDelta;
		public float blendIn;
		public float blendOut;
		public float speed;
		public float sine2DMagnitudeMin;
		public float sine2DMagnitudeMax;
		public float sine2DFrequency;
		public float sine2DOffsetAngle;
		public uint localRadial;
		public float localMagnitude;
	}
}
