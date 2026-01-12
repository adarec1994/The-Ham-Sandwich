namespace ProjectWS.Engine.Database.Definitions
{
	public class ArchiveCategory : TblRecord
	{
		public override string GetFileName() => "ArchiveCategory";
		public override uint GetID() => this.ID;
		
		public uint ID;
		public uint localizedTextId;
	}
}
