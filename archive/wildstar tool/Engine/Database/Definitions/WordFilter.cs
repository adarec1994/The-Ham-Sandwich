namespace ProjectWS.Engine.Database.Definitions
{
	public class WordFilter : TblRecord
	{
		public override string GetFileName() => "WordFilter";
		public override uint GetID() => this.ID;
		
		public uint ID;
		public string filter;
		public uint userTextFilterClassEnum;
		public uint deploymentRegionEnum;
		public uint languageId;
		public uint wordFilterTypeEnum;
	}
}
