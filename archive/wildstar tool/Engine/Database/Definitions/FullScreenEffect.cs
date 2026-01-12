namespace ProjectWS.Engine.Database.Definitions
{
	public class FullScreenEffect : TblRecord
	{
		public override string GetFileName() => "FullScreenEffect";
		public override uint GetID() => this.ID;
		
		public uint ID;
		public string description;
		public string texturePath;
		public string modelPath;
		public uint priority;
		public uint fullScreenEffectTypeEnum;
		public float alphaMinStart;
		public float alphaMinEnd;
		public float alphaStart;
		public float alphaEnd;
		public float hzStart;
		public float hzEnd;
		public float saturationStart;
		public float saturationEnd;
	}
}
