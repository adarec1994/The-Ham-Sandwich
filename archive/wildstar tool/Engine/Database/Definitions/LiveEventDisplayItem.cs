namespace ProjectWS.Engine.Database.Definitions
{
	public class LiveEventDisplayItem : TblRecord
	{
		public override string GetFileName() => "LiveEventDisplayItem";
		public override uint GetID() => this.ID;
		
		public uint ID;
		public uint liveEventId;
		public uint item2Id;
		public uint storeLinkId;
	}
}
