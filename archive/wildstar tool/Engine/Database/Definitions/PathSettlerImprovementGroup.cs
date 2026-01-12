namespace ProjectWS.Engine.Database.Definitions
{
	public class PathSettlerImprovementGroup : TblRecord
	{
		public override string GetFileName() => "PathSettlerImprovementGroup";
		public override uint GetID() => this.ID;
		
		public uint ID;
		public uint pathSettlerHubId;
		public uint pathSettlerImprovementGroupFlags;
		public uint creature2IdDepot;
		public uint localizedTextIdName;
		public uint settlerAvenueTypeEnum;
		public uint contributionValue;
		public uint perTierBonusContributionValue;
		public uint durationPerBundleMs;
		public uint maxBundleCount;
		public uint pathSettlerImprovementGroupIdOutpostRequired;
		public uint pathSettlerImprovementIdTier00;
		public uint pathSettlerImprovementIdTier01;
		public uint pathSettlerImprovementIdTier02;
		public uint pathSettlerImprovementIdTier03;
		public uint worldLocation2IdDisplayPoint;
	}
}
