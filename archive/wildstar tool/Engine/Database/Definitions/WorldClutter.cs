namespace ProjectWS.Engine.Database.Definitions
{
	public class WorldClutter : TblRecord
	{
		public override string GetFileName() => "WorldClutter";
		public override uint GetID() => this.ID;
		
		public uint ID;
		public string Description;
		public float density;
		public uint clutterFlags;
		public string assetPath0;
		public string assetPath01;
		public string assetPath02;
		public string assetPath03;
		public string assetPath04;
		public string assetPath05;
		public float assetWeight0;
		public float assetWeight01;
		public float assetWeight02;
		public float assetWeight03;
		public float assetWeight04;
		public float assetWeight05;
		public uint flag0;
		public uint flag01;
		public uint flag02;
		public uint flag03;
		public uint flag04;
		public uint flag05;
		public float minScale0;
		public float minScale01;
		public float minScale02;
		public float minScale03;
		public float minScale04;
		public float minScale05;
		public float rotationMin0;
		public float rotationMin01;
		public float rotationMin02;
		public float rotationMin03;
		public float rotationMin04;
		public float rotationMin05;
		public float rotationMax0;
		public float rotationMax01;
		public float rotationMax02;
		public float rotationMax03;
		public float rotationMax04;
		public float rotationMax05;
		public uint emissiveGlow00;
		public uint emissiveGlow01;
		public uint emissiveGlow02;
		public uint emissiveGlow03;
		public uint emissiveGlow04;
		public uint emissiveGlow05;
	}
}
