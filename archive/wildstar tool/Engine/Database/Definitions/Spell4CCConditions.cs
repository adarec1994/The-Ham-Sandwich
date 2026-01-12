namespace ProjectWS.Engine.Database.Definitions
{
	public class Spell4CCConditions : TblRecord
	{
		public override string GetFileName() => "Spell4CCConditions";
		public override uint GetID() => this.ID;
		
		public uint ID;
		public uint ccStateMask;
		public uint ccStateFlagsRequired;
	}
}
