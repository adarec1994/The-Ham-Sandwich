namespace ProjectWS.Engine.Database.Definitions
{
	public class HousingContributionInfo : TblRecord
	{
		public override string GetFileName() => "HousingContributionInfo";
		public override uint GetID() => this.ID;
		
		public uint ID;
		public uint housingContributionTypeId;
		public uint contributionPointRequirement;
		public uint item2IdTier00;
		public uint item2IdTier01;
		public uint item2IdTier02;
		public uint item2IdTier03;
		public uint item2IdTier04;
		public uint contributionPointValueTier00;
		public uint contributionPointValueTier01;
		public uint contributionPointValueTier02;
		public uint contributionPointValueTier03;
		public uint contributionPointValueTier04;
	}
}
