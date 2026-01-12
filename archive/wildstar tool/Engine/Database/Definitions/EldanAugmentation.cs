namespace ProjectWS.Engine.Database.Definitions
{
	public class EldanAugmentation : TblRecord
	{
		public override string GetFileName() => "EldanAugmentation";
		public override uint GetID() => this.ID;
		
		public uint ID;
		public uint displayRow;
		public uint displayColumn;
		public uint classId;
		public uint powerCost;
		public uint eldanAugmentationIdRequired;
		public uint spell4IdAugment;
		public uint item2IdUnlock;
		public uint eldanAugmentationCategoryId;
		public uint categoryTier;
		public uint localizedTextIdTitle;
		public uint localizedTextIdTooltip;
	}
}
