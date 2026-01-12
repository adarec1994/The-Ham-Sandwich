namespace ProjectWS.Engine.Database.Definitions
{
	public class SoundCombatLoop : TblRecord
	{
		public override string GetFileName() => "SoundCombatLoop";
		public override uint GetID() => this.ID;
		
		public uint ID;
		public uint soundEventIdStart;
		public uint soundEventIdStop;
		public uint soundParameterIdUnitsInCombat;
	}
}
