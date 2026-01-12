namespace ProjectWS.Engine.Database.Definitions
{
	public class Spell4Tag : TblRecord
	{
		public override string GetFileName() => "Spell4Tag";
		public override uint GetID() => this.ID;
		
		public uint ID;
		public uint flags;
		public uint localizedTextIdName;
	}
}
