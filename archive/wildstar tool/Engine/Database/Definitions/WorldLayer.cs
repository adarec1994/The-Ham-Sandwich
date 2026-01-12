namespace ProjectWS.Engine.Database.Definitions
{
	public class WorldLayer : TblRecord
	{
		public override string GetFileName() => "WorldLayer";
		public override uint GetID() => this.ID;
		
		public uint ID;
		public string Description;
		public float HeightScale;
		public float HeightOffset;
		public float ParallaxScale;
		public float ParallaxOffset;
		public float MetersPerTextureTile;
		public string ColorMapPath;
		//public uint padding;
		public string NormalMapPath;
		public uint AverageColor;
		public uint Projection;
		public uint materialType;
		public uint worldClutterId00;
		public uint worldClutterId01;
		public uint worldClutterId02;
		public uint worldClutterId03;
		public float specularPower;
		public uint emissiveGlow;
		public float scrollSpeed00;
		public float scrollSpeed01;
	}
}
