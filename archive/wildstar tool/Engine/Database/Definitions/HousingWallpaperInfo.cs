namespace ProjectWS.Engine.Database.Definitions
{
	public class HousingWallpaperInfo : TblRecord
	{
		public override string GetFileName() => "HousingWallpaperInfo";
		public override uint GetID() => this.ID;
		
		public uint ID;
		public uint localizedTextId;
		public uint cost;
		public uint costCurrencyTypeId;
		public uint replaceableMaterialInfoId;
		public uint worldSkyId;
		public uint flags;
		public uint prerequisiteIdUnlock;
		public uint prerequisiteIdUse;
		public uint unlockIndex;
		public uint soundZoneKitId;
		public uint worldLayerId00;
		public uint worldLayerId01;
		public uint worldLayerId02;
		public uint worldLayerId03;
		public uint accountItemIdUpsell;
	}
}
