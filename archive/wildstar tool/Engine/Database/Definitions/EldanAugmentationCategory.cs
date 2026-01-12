namespace ProjectWS.Engine.Database.Definitions
{
	public class EldanAugmentationCategory : TblRecord
	{
		public override string GetFileName() => "EldanAugmentationCategory";
		public override uint GetID() => this.ID;
		
		public uint ID;
		public uint eldanAugmentationCategoryIdTier2Category00;
		public uint eldanAugmentationCategoryIdTier2Category01;
		public uint tier2CostAmount00;
		public uint tier2CostAmount01;
		public uint tier3CostAmount00;
		public uint tier3CostAmount01;
		public uint localizedTextIdName;
	}
}
