namespace ProjectWS.Engine.Database.Definitions
{
	public class WorldWaterLayer : TblRecord
	{
		public override string GetFileName() => "WorldWaterLayer";
		public override uint GetID() => this.ID;
		
		public uint ID;
		public string description;
		public string RippleColorTex;
		public string RippleNormalTex;
		public float Scale;
		public float Rotation;
		public float Speed;
		public float OscFrequency;
		public float OscMagnitude;
		public float OscRotation;
		public float OscPhase;
		public float OscMinLayerWeight;
		public float OscMaxLayerWeight;
		public float OscLayerWeightPhase;
		public float materialBlend;
	}
}
