namespace ProjectWS.Engine.Database.Definitions
{
	public class TaxiNode : TblRecord
	{
		public override string GetFileName() => "TaxiNode";
		public override uint GetID() => this.ID;
		
		public uint ID;
		public uint localizedTextId;
		public uint taxiNodeTypeEnum;
		public uint flags;
		public uint flightPathTypeEnum;
		public uint taxiNodeFactionEnum;
		public uint worldLocation2Id;
		public uint contentTier;
		public uint autoUnlockLevel;
		public uint recommendedMinLevel;
		public uint recommendedMaxLevel;
	}
}
