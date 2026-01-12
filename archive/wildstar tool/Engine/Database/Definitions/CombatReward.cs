namespace ProjectWS.Engine.Database.Definitions
{
	public class CombatReward : TblRecord
	{
		public override string GetFileName() => "CombatReward";
		public override uint GetID() => this.ID;
		
		public uint ID;
		public uint combatRewardTypeEnum;
		public uint localizedTextIdCombatFloater;
		public uint localizedTextIdCombatLogMessage;
		public uint visualEffectIdVisual00;
		public uint visualEffectIdVisual01;
	}
}
