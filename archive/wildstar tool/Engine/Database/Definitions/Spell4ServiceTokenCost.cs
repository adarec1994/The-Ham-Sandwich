namespace ProjectWS.Engine.Database.Definitions
{
	public class Spell4ServiceTokenCost : TblRecord
	{
		public override string GetFileName() => "Spell4ServiceTokenCost";
		public override uint GetID() => this.ID;
		
		public uint ID;
		public uint spell4Id;
		public uint serviceTokenCost;
	}
}
