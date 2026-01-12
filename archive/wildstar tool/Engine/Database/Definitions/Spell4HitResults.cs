namespace ProjectWS.Engine.Database.Definitions
{
	public class Spell4HitResults : TblRecord
	{
		public override string GetFileName() => "Spell4HitResults";
		public override uint GetID() => this.ID;
		
		public uint ID;
		public uint flags;
	}
}
