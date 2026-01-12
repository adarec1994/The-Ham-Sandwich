namespace ProjectWS.Engine.Database.Definitions
{
	public class GossipSet : TblRecord
	{
		public override string GetFileName() => "GossipSet";
		public override uint GetID() => this.ID;
		
		public uint ID;
		public uint flags;
		public uint gossipProximityEnum;
		public uint cooldown;
	}
}
