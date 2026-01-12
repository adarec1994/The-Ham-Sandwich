namespace ProjectWS.Engine.Database.Definitions
{
	public class SpellCoolDown : TblRecord
	{
		public override string GetFileName() => "SpellCoolDown";
		public override uint GetID() => this.ID;
		
		public uint ID;
		public uint cooldownTime;
	}
}
