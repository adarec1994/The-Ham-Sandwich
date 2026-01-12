namespace ProjectWS.Engine.Database.Definitions
{
	public class PublicEventObjective : TblRecord
	{
		public override string GetFileName() => "PublicEventObjective";
		public override uint GetID() => this.ID;
		
		public uint ID;
		public uint publicEventId;
		public uint publicEventObjectiveFlags;
		public uint publicEventObjectiveTypeSpecificFlags;
		public uint worldLocation2Id;
		public uint publicEventTeamId;
		public uint localizedTextId;
		public uint localizedTextIdOtherTeam;
		public uint localizedTextIdShort;
		public uint localizedTextIdOtherTeamShort;
		public uint publicEventObjectiveTypeEnum;
		public uint count;
		public uint objectId;
		public uint failureTimeMs;
		public uint targetGroupIdRewardPane;
		public uint publicEventObjectiveCategoryEnum;
		public uint liveEventIdCounter;
		public uint publicEventObjectiveIdParent;
		public uint questDirectionId;
		public uint medalPointValue;
		public uint localizedTextIdParticipantAdd;
		public uint localizedTextIdStart;
		public uint displayOrder;
	}
}
