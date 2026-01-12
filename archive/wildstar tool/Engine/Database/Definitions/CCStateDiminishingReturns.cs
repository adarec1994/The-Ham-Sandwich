namespace ProjectWS.Engine.Database.Definitions
{
	public class CCStateDiminishingReturns : TblRecord
	{
		public override string GetFileName() => "CCStateDiminishingReturns";
		public override uint GetID() => this.ID;
		
		public uint ID;
		public uint localizedTextIdName;
	}
}
