namespace ProjectWS.Engine.Database.Definitions
{
	public class MatchTypeRewardRotationContent : TblRecord
	{
		public override string GetFileName() => "MatchTypeRewardRotationContent";
		public override uint GetID() => this.ID;
		
		public uint ID;
		public uint matchTypeEnum;
		public uint rewardRotationContentIdRandomNormal;
		public uint rewardRotationContentIdRandomVeteran;
	}
}
