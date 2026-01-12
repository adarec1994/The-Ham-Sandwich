namespace ProjectWS.Engine.Database.Definitions
{
	public class BugCategory : TblRecord
	{
		public override string GetFileName() => "BugCategory";
		public override uint GetID() => this.ID;
		
		public uint ID;
		public uint localizedTextId;
	}
}
