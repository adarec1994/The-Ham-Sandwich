namespace ProjectWS.Engine.Database.Definitions
{
	public class TicketSubCategory : TblRecord
	{
		public override string GetFileName() => "TicketSubCategory";
		public override uint GetID() => this.ID;
		
		public uint ID;
		public uint ticketCategoryId;
		public uint localizedTextId;
	}
}
