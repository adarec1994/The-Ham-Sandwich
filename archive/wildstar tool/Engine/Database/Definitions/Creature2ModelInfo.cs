namespace ProjectWS.Engine.Database.Definitions
{
	public class Creature2ModelInfo : TblRecord
	{
		public override string GetFileName() => "Creature2ModelInfo";
		public override uint GetID() => this.ID;
		
		public uint ID;
		public string assetPath;
		public string assetTexture0;
		public string assetTexture1;
		public uint modelTextureId0;
		public uint modelTextureId1;
		public uint creatureMaterialEnum;
		public float scale;
		public float hitRadius;
		public float walkLand;
		public float walkAir;
		public float walkWater;
		public float walkHover;
		public float runLand;
		public float runAir;
		public float runWater;
		public float runHover;
		public uint itemVisualTypeIdFeet;
		public float swimWaterDepth;
		public uint race;
		public uint sex;
		public uint itemDisplayId00;
		public uint itemDisplayId01;
		public uint itemDisplayId02;
		public uint itemDisplayId03;
		public uint itemDisplayId04;
		public uint itemDisplayId05;
		public uint itemDisplayId06;
		public uint itemDisplayId07;
		public uint itemDisplayId08;
		public uint itemDisplayId09;
		public uint itemDisplayId10;
		public uint itemDisplayId11;
		public uint itemDisplayId12;
		public uint itemDisplayId13;
		public uint itemDisplayId14;
		public uint itemDisplayId15;
		public uint itemDisplayId16;
		public uint itemDisplayId17;
		public uint itemDisplayId18;
		public uint itemDisplayId19;
		public uint modelMeshId00;
		public uint modelMeshId01;
		public uint modelMeshId02;
		public uint modelMeshId03;
		public uint modelMeshId04;
		public uint modelMeshId05;
		public uint modelMeshId06;
		public uint modelMeshId07;
		public uint modelMeshId08;
		public uint modelMeshId09;
		public uint modelMeshId10;
		public uint modelMeshId11;
		public uint modelMeshId12;
		public uint modelMeshId13;
		public uint modelMeshId14;
		public uint modelMeshId15;
		public float groundOffsetHover;
		public float groundOffsetFly;
	}
}
