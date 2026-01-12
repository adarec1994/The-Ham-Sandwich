namespace ProjectWS.Engine.Database.Definitions
{
	public class TradeskillTier : TblRecord
	{
		public override string GetFileName() => "TradeskillTier";
		public override uint GetID() => this.ID;
		
		public uint ID;
		public uint tradeSkillId;
		public uint tier;
		public uint requiredXp;
		public uint learnXp;
		public uint craftXp;
		public uint firstCraftXp;
		public uint questXp;
		public uint failXp;
		public uint itemLevelMin;
		public uint maxAdditives;
		public ulong relearnCost;
		public uint achievementCategoryId;
	}
}
