namespace ProjectWS.Engine.Database.Definitions
{
	public class PublicEventRewardModifier : TblRecord
	{
		public override string GetFileName() => "PublicEventRewardModifier";
		public override uint GetID() => this.ID;
		
		public uint ID;
		public uint publicEventId;
		public uint rewardPropertyId;
		public uint data;
		public float offset;
	}
}
