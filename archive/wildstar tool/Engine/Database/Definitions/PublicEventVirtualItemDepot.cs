namespace ProjectWS.Engine.Database.Definitions
{
	public class PublicEventVirtualItemDepot : TblRecord
	{
		public override string GetFileName() => "PublicEventVirtualItemDepot";
		public override uint GetID() => this.ID;
		
		public uint ID;
		public uint creature2Id;
		public uint virtualItemId00;
		public uint virtualItemId01;
		public uint virtualItemId02;
		public uint virtualItemId03;
		public uint virtualItemId04;
		public uint virtualItemId05;
	}
}
