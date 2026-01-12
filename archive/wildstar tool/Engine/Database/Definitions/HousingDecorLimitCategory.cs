namespace ProjectWS.Engine.Database.Definitions
{
	public class HousingDecorLimitCategory : TblRecord
	{
		public override string GetFileName() => "HousingDecorLimitCategory";
		public override uint GetID() => this.ID;
		
		public uint ID;
		public uint decorLimit;
	}
}
