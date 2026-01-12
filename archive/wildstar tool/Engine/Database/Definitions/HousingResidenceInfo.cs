namespace ProjectWS.Engine.Database.Definitions
{
	public class HousingResidenceInfo : TblRecord
	{
		public override string GetFileName() => "HousingResidenceInfo";
		public override uint GetID() => this.ID;
		
		public uint ID;
		public uint housingDecorInfoIdDefaultRoof;
		public uint housingDecorInfoIdDefaultEntryway;
		public uint housingDecorInfoIdDefaultDoor;
		public uint housingWallpaperInfoIdDefault;
		public uint worldLocation2IdInside00;
		public uint worldLocation2IdInside01;
		public uint worldLocation2IdInside02;
		public uint worldLocation2IdInside03;
		public uint worldLocation2IdOutside00;
		public uint worldLocation2IdOutside01;
		public uint worldLocation2IdOutside02;
		public uint worldLocation2IdOutside03;
	}
}
