namespace ProjectWS.Engine.Database.Definitions
{
	public class XpPerLevel : TblRecord
	{
		public override string GetFileName() => "XpPerLevel";
		public override uint GetID() => this.ID;
		
		public uint ID;
		public uint minXpForLevel;
		public uint baseQuestXpPerLevel;
		public uint abilityPointsPerLevel;
		public uint attributePointsPerLevel;
		public uint baseRepRewardPerLevel;
	}
}
