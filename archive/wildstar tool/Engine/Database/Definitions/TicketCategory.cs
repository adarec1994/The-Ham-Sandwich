namespace ProjectWS.Engine.Database.Definitions
{
	public class TicketCategory : TblRecord
	{
		public override string GetFileName() => "TicketCategory";
		public override uint GetID() => this.ID;
		
		public uint ID;
		public uint localizedTextId;
	}
}
