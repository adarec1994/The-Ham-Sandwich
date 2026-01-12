namespace ProjectWS.Engine.Database.Definitions
{
	public class Challenge : TblRecord
	{
		public override string GetFileName() => "Challenge";
		public override uint GetID() => this.ID;
		
		public uint ID;
		public uint challengeTypeEnum;
		public uint target;
		public uint challengeFlags;
		public uint worldZoneIdRestriction;
		public uint triggerVolume2IdRestriction;
		public uint worldZoneId;
		public uint worldLocation2IdIndicator;
		public uint worldLocation2IdStartLocation;
		public uint completionCount;
		public uint challengeTierId00;
		public uint challengeTierId01;
		public uint challengeTierId02;
		public uint localizedTextIdName;
		public uint localizedTextIdProgress;
		public uint localizedTextIdAreaRestriction;
		public uint localizedTextIdLocation;
		public uint virtualItemIdDisplay;
		public uint targetGroupIdRewardPane;
		public uint questDirectionIdActive;
		public uint questDirectionIdInactive;
		public uint rewardTrackId;
	}
}
