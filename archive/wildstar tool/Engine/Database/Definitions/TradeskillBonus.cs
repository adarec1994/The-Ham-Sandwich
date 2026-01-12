namespace ProjectWS.Engine.Database.Definitions
{
	public class TradeskillBonus : TblRecord
	{
		public override string GetFileName() => "TradeskillBonus";
		public override uint GetID() => this.ID;
		
		public uint ID;
		public uint tradeSkillTierId;
		public uint achievementId;
		public string iconPath;
		public uint localizedTextIdName;
		public uint localizedTextIdTooltip;
		public uint tradeskillBonusEnum00;
		public uint tradeskillBonusEnum01;
		public uint tradeskillBonusEnum02;
		public uint objectIdPrimary00;
		public uint objectIdPrimary01;
		public uint objectIdPrimary02;
		public uint objectIdSecondary00;
		public uint objectIdSecondary01;
		public uint objectIdSecondary02;
		public uint objectIdTertiary00;
		public uint objectIdTertiary01;
		public uint objectIdTertiary02;
		public float value00;
		public float value01;
		public float value02;
		public uint valueInt00;
		public uint valueInt01;
		public uint valueInt02;
	}
}
