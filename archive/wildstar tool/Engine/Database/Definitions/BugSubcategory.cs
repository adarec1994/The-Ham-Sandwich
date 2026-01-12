namespace ProjectWS.Engine.Database.Definitions
{
	public class BugSubcategory : TblRecord
	{
		public override string GetFileName() => "BugSubcategory";
		public override uint GetID() => this.ID;
		
		public uint ID;
		public uint bugCategoryId;
		public uint localizedTextId;
	}
}
