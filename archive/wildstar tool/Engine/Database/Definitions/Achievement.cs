namespace ProjectWS.Engine.Database.Definitions
{
	public class Achievement : TblRecord
	{
		public override string GetFileName() => "Achievement";
		public override uint GetID() => this.ID;
		
		public uint ID;
		public uint achievementTypeId;
		public uint achievementCategoryId;
		public uint flags;
		public uint worldZoneId;
		public uint localizedTextIdTitle;
		public uint localizedTextIdDesc;
		public uint localizedTextIdProgress;
		public float percCompletionToShow;
		public uint objectId;
		public uint objectIdAlt;
		public uint value;
		public uint characterTitleId;
		public uint prerequisiteId;
		public uint prerequisiteIdServer;
		public uint prerequisiteIdObjective;
		public uint prerequisiteIdObjectiveAlt;
		public uint achievementIdParentTier;
		public uint orderIndex;
		public uint achievementGroupId;
		public uint achievementSubGroupId;
		public uint achievementPointEnum;
		public string steamAchievementName;
	}
}
