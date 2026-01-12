namespace ProjectWS.Engine.Database.Definitions
{
	public class Spell4ValidTargets : TblRecord
	{
		public override string GetFileName() => "Spell4ValidTargets";
		public override uint GetID() => this.ID;
		
		public uint ID;
		public uint targetBitmask;
	}
}
