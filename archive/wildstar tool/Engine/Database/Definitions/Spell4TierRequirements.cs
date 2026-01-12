namespace ProjectWS.Engine.Database.Definitions
{
	public class Spell4TierRequirements : TblRecord
	{
		public override string GetFileName() => "Spell4TierRequirements";
		public override uint GetID() => this.ID;
		
		public uint ID;
		public uint tierIndex;
		public uint levelRequirement;
	}
}
