namespace ProjectWS.Engine.Database.Definitions
{
	public class RewardTrack : TblRecord
	{
		public override string GetFileName() => "RewardTrack";
		public override uint GetID() => this.ID;
		
		public uint ID;
		public uint rewardTrackTypeEnum;
		public uint prerequisiteId;
		public uint localizedTextId;
		public uint localizedTextIdSummary;
		public string assetPathImage;
		public string assetPathButtonImage;
		public uint rewardPointCost00;
		public uint rewardPointCost01;
		public uint rewardPointCost02;
		public uint rewardPointCost03;
		public uint rewardPointCost04;
		public uint rewardPointCost05;
		public uint rewardPointCost06;
		public uint rewardPointCost07;
		public uint rewardPointCost08;
		public uint rewardPointCost09;
		public uint rewardTrackIdParent;
		public uint flags;
	}
}
