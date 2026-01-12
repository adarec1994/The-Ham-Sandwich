namespace ProjectWS.Engine.Database.Definitions
{
	public class CCStates : TblRecord
	{
		public override string GetFileName() => "CCStates";
		public override uint GetID() => this.ID;
		
		public uint ID;
		public uint flags;
		public float defaultBreakProbability;
		public uint localizedTextIdName;
		public string spellIcon;
		public uint visualEffectId00;
		public uint visualEffectId01;
		public uint visualEffectId02;
		public uint ccStateDiminishingReturnsId;
	}
}
