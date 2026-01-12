namespace ProjectWS.Engine.Database.Definitions
{
	public class PublicEventDepot : TblRecord
	{
		public override string GetFileName() => "PublicEventDepot";
		public override uint GetID() => this.ID;
		
		public uint ID;
		public uint creature2Id;
		public uint item2Id;
	}
}
