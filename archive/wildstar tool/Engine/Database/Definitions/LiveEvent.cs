namespace ProjectWS.Engine.Database.Definitions
{
	public class LiveEvent : TblRecord
	{
		public override string GetFileName() => "LiveEvent";
		public override uint GetID() => this.ID;
		
		public uint ID;
		public uint liveEventTypeEnum;
		public uint maxValue;
		public uint flags;
		public uint liveEventCategoryEnum;
		public uint liveEventIdParent;
		public uint localizedTextIdName;
		public uint localizedTextIdSummary;
		public string iconPath;
		public string iconPathButton;
		public string spritePathTitle;
		public string spritePathBackground;
		public uint currencyTypeIdEarned;
		public uint localizedTextIdCurrencyEarnedTooltip;
		public uint worldLocation2IdExile;
		public uint worldLocation2IdDominion;
	}
}
