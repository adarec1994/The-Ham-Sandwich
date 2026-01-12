namespace ProjectWS.Engine.Database.Definitions
{
	public class Spell4SpellTypes : TblRecord
	{
		public override string GetFileName() => "Spell4SpellTypes";
		public override uint GetID() => this.ID;
		
		public uint ID;
		public string typeName;
		public string enumName;
	}
}
