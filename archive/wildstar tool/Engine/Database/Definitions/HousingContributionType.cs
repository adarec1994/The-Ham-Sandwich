namespace ProjectWS.Engine.Database.Definitions
{
	public class HousingContributionType : TblRecord
	{
		public override string GetFileName() => "HousingContributionType";
		public override uint GetID() => this.ID;
		
		public uint ID;
		public string description;
		public string enumName;
	}
}
