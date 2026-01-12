namespace ProjectWS.Engine.Database.Definitions
{
	public class HousingPropertyInfo : TblRecord
	{
		public override string GetFileName() => "HousingPropertyInfo";
		public override uint GetID() => this.ID;
		
		public uint ID;
		public uint localizedTextIdName;
		public uint worldId;
		public uint housingMapInfoId;
		public uint cost;
		public uint housingFacingEnum;
		public uint worldLocation2Id;
		public uint worldZoneId;
		public uint housingPropertyTypeId;
		public uint worldLayerIdDefault00;
		public uint worldLayerIdDefault01;
		public uint worldLayerIdDefault02;
		public uint worldLayerIdDefault03;
	}
}
