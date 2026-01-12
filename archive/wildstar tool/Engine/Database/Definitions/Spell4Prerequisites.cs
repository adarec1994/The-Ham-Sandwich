namespace ProjectWS.Engine.Database.Definitions
{
	public class Spell4Prerequisites : TblRecord
	{
		public override string GetFileName() => "Spell4Prerequisites";
		public override uint GetID() => this.ID;
		
		public uint ID;
		public uint flags;
	}
}
