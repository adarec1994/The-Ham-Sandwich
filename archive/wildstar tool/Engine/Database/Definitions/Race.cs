namespace ProjectWS.Engine.Database.Definitions
{
	public class Race : TblRecord
	{
		public override string GetFileName() => "Race";
		public override uint GetID() => this.ID;
		
		public uint ID;
		public string enumName;
		public uint localizedTextId;
		public uint localizedTextIdNameFemale;
		public string maleAssetPath;
		public string femaleAssetPath;
		public float hitRadius;
		public uint soundImpactDescriptionIdOrigin;
		public uint soundImpactDescriptionIdTarget;
		public float walkLand;
		public float walkAir;
		public float walkWater;
		public float walkHover;
		public uint unitVisualTypeIdMale;
		public uint unitVisualTypeIdFemale;
		public uint soundEventIdMaleHealthStart;
		public uint soundEventIdFemaleHealthStart;
		public uint soundEventIdMaleHealthStop;
		public uint soundEventIdFemaleHealthStop;
		public float swimWaterDepth;
		public uint itemDisplayIdUnderwearLegsMale;
		public uint itemDisplayIdUnderwearLegsFemale;
		public uint itemDisplayIdUnderwearChestFemale;
		public uint itemDisplayIdArmCannon;
		public float mountScaleMale;
		public float mountScaleFemale;
		public uint soundSwitchId;
		public uint componentLayoutIdMale;
		public uint componentLayoutIdFemale;
		public uint modelMeshIdMountItemMale;
		public uint modelMeshIdMountItemFemale;
	}
}
