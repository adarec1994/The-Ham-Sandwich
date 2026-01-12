namespace ProjectWS.Engine.Database.Definitions
{
	public class ItemProficiency : TblRecord
	{
		public override string GetFileName() => "ItemProficiency";
		public override uint GetID() => this.ID;
		
		public uint ID;
		public uint bitMask;
		public uint localizedTextIdString;
	}
}
