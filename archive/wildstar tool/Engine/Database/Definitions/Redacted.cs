namespace ProjectWS.Engine.Database.Definitions
{
	public class Redacted : TblRecord
	{
		public override string GetFileName() => "Redacted";
		public override uint GetID() => this.ID;
		
		public uint ID;
		public string idString;
		public uint localizedTextIdName;
		public uint mtxCategoryIdParent;
	}
}
