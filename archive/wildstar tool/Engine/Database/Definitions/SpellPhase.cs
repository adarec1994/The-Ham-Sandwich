namespace ProjectWS.Engine.Database.Definitions
{
	public class SpellPhase : TblRecord
	{
		public override string GetFileName() => "SpellPhase";
		public override uint GetID() => this.ID;
		
		public uint ID;
		public uint phaseDelay;
		public uint phaseFlags;
		public uint orderIndex;
		public uint spell4IdOwner;
	}
}
