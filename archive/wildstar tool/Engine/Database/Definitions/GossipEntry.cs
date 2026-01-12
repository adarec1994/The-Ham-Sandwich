namespace ProjectWS.Engine.Database.Definitions
{
	public class GossipEntry : TblRecord
	{
		public override string GetFileName() => "GossipEntry";
		public override uint GetID() => this.ID;
		
		public uint ID;
		public uint gossipSetId;
		public uint indexOrder;
		public uint localizedTextId;
		public uint prerequisiteId;
	}
}
