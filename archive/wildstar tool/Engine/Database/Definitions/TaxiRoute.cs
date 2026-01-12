namespace ProjectWS.Engine.Database.Definitions
{
	public class TaxiRoute : TblRecord
	{
		public override string GetFileName() => "TaxiRoute";
		public override uint GetID() => this.ID;
		
		public uint ID;
		public uint taxiNodeIdSource;
		public uint taxiNodeIdDestination;
		public uint price;
	}
}
