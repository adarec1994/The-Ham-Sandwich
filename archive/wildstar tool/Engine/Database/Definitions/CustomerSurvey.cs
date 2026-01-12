namespace ProjectWS.Engine.Database.Definitions
{
	public class CustomerSurvey : TblRecord
	{
		public override string GetFileName() => "CustomerSurvey";
		public override uint GetID() => this.ID;
		
		public uint ID;
		public uint customerSurveyTypeEnum;
		public uint localizedTextIdOverrideTitle;
		public uint localizedTextIdQuestion00;
		public uint localizedTextIdQuestion01;
		public uint localizedTextIdQuestion02;
	}
}
