namespace ProjectWS.Engine.Database.Definitions
{
	public class WorldWaterWake : TblRecord
	{
		public override string GetFileName() => "WorldWaterWake";
		public override uint GetID() => this.ID;
		
		public uint ID;
		public uint flags;
		public string colorTexture;
		public string normalTexture;
		public string distortionTexture;
		public uint durationMin;
		public uint durationMax;
		public float scaleStart;
		public float scaleEnd;
		public float alphaStart;
		public float alphaEnd;
		public float distortionWeight;
		public float distortionScaleStart;
		public float distortionScaleEnd;
		public float distortionSpeedU;
		public float distortionSpeedV;
		public float positionOffsetX;
		public float positionOffsetY;
	}
}
