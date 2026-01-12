namespace ProjectWS.Engine.Database.Definitions
{
	public class HousingNeighborhoodInfo : TblRecord
	{
		public override string GetFileName() => "HousingNeighborhoodInfo";
		public override uint GetID() => this.ID;
		
		public uint ID;
		public uint baseCost;
		public uint maxPopulation;
		public uint populationThreshold;
		public uint housingFactionEnum;
		public uint housingFeatureTypeEnum;
		public uint housingPlaystyleTypeEnum;
		public uint housingMapInfoIdPrimary;
		public uint housingMapInfoIdSecondary;
	}
}
