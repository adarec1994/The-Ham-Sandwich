namespace ProjectWS.Engine.Database.Definitions
{
	public class WordFilterAlt : TblRecord
	{
		public override string GetFileName() => "WordFilterAlt";
		public override uint GetID() => this.ID;
		
		public uint ID;
		public string filter;
	}
}
