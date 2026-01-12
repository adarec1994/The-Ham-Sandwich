namespace ProjectWS.Engine.Database.Definitions
{
	public class HousingPlotInfo : TblRecord
	{
		public override string GetFileName() => "HousingPlotInfo";
		public override uint GetID() => this.ID;
		
		public uint ID;
		public uint worldSocketId;
		public uint plotType;
		public uint housingPropertyInfoId;
		public uint housingPropertyPlotIndex;
		public uint housingPlugItemIdDefault;
	}
}
