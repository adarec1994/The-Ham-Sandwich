namespace ProjectWS.Engine.Database.Definitions
{
	public class HousingMapInfo : TblRecord
	{
		public override string GetFileName() => "HousingMapInfo";
		public override uint GetID() => this.ID;
		
		public uint ID;
		public uint worldId;
		public uint privatePropertyCount;
		public uint publicPropertyCount;
	}
}
