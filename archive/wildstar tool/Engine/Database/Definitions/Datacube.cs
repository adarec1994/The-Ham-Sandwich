namespace ProjectWS.Engine.Database.Definitions
{
	public class Datacube : TblRecord
	{
		public override string GetFileName() => "Datacube";
		public override uint GetID() => this.ID;
		
		public uint ID;
		public uint datacubeTypeEnum;
		public uint localizedTextIdTitle;
		public uint localizedTextIdText00;
		public uint localizedTextIdText01;
		public uint localizedTextIdText02;
		public uint localizedTextIdText03;
		public uint localizedTextIdText04;
		public uint localizedTextIdText05;
		public uint soundEventId;
		public uint worldZoneId;
		public uint unlockCount;
		public string assetPathImage;
		public uint datacubeFactionEnum;
		public uint worldLocation2Id;
		public uint questDirectionId;
	}
}
