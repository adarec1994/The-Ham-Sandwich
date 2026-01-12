namespace ProjectWS.Engine.Database.Definitions
{
	public class HousingPlugItem : TblRecord
	{
		public override string GetFileName() => "HousingPlugItem";
		public override uint GetID() => this.ID;
		
		public uint ID;
		public uint localizedTextIdName;
		public uint housingPlotTypeId;
		public uint localizedTextIdTooltip;
		public uint worldIdPlug00;
		public uint worldIdPlug01;
		public uint worldIdPlug02;
		public uint worldIdPlug03;
		public uint flags;
		public uint housingResourceIdProvided00;
		public uint housingResourceIdProvided01;
		public uint housingResourceIdProvided02;
		public uint housingResourceIdProvided03;
		public uint housingResourceIdProvided04;
		public uint housingResourceIdPrerequisite00;
		public uint housingResourceIdPrerequisite01;
		public uint housingResourceIdPrerequisite02;
		public uint housingFeatureTypeFlags;
		public uint housingContributionInfoId00;
		public uint housingContributionInfoId01;
		public uint housingContributionInfoId02;
		public uint housingContributionInfoId03;
		public uint housingContributionInfoId04;
		public uint housingPlugItemIdNextUpgrade;
		public uint localizedTextIdPrereqTooltip00;
		public uint localizedTextIdPrereqTooltip01;
		public uint localizedTextIdPrereqTooltip02;
		public uint prerequisiteId00;
		public uint prerequisiteId01;
		public uint prerequisiteId02;
		public uint prerequisiteIdUnlock;
		public uint housingBuildId;
		public uint housingUpkeepTypeEnum;
		public uint upkeepCharges;
		public float upkeepTime;
		public uint housingContributionInfoIdUpkeepCost00;
		public uint housingContributionInfoIdUpkeepCost01;
		public uint housingContributionInfoIdUpkeepCost02;
		public uint housingContributionInfoIdUpkeepCost03;
		public uint housingContributionInfoIdUpkeepCost04;
		public uint gameFormulaIdHousingBuildBonus;
		public string screenshotSprite00;
		public string screenshotSprite01;
		public string screenshotSprite02;
		public string screenshotSprite03;
		public string screenshotSprite04;
		public uint housingPlugTypeEnum;
		public uint accountItemIdUpsell;
	}
}
