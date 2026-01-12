namespace ProjectWS.Engine.Database.Definitions
{
	public class ItemSpecial : TblRecord
	{
		public override string GetFileName() => "ItemSpecial";
		public override uint GetID() => this.ID;
		
		public uint ID;
		public uint prerequisiteIdGeneric00;
		public uint localizedTextIdName;
		public uint spell4IdOnEquip;
		public uint spell4IdOnActivate;
	}
}
