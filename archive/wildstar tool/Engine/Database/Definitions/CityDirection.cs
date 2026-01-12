namespace ProjectWS.Engine.Database.Definitions
{
	public class CityDirection : TblRecord
	{
		public override string GetFileName() => "CityDirection";
		public override uint GetID() => this.ID;
		
		public uint ID;
		public uint cityDirectionTypeEnum;
		public uint localizedTextIdName;
		public uint worldZoneId;
		public uint worldLocation2Id00;
		public uint worldLocation2Id01;
		public uint worldLocation2Id02;
		public uint worldLocation2Id03;
	}
}
