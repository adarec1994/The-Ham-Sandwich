namespace ProjectWS.Engine.Database.Definitions
{
	public class Language : TblRecord
	{
		public override string GetFileName() => "Language";
		public override uint GetID() => this.ID;
		
		public uint ID;
		public string languageTag;
		public string clientLanguageTag;
		public uint soundAvailabilityIndexFemale;
	}
}
