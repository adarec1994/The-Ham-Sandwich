namespace ProjectWS.Engine.Database.Definitions
{
	public class ItemRandomStatGroup : TblRecord
	{
		public override string GetFileName() => "ItemRandomStatGroup";
		public override uint GetID() => this.ID;
		
		public uint ID;
		public uint localizedTextIdName;
	}
}
