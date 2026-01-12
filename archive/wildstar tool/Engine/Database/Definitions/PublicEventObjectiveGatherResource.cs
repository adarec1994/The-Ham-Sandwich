namespace ProjectWS.Engine.Database.Definitions
{
	public class PublicEventObjectiveGatherResource : TblRecord
	{
		public override string GetFileName() => "PublicEventObjectiveGatherResource";
		public override uint GetID() => this.ID;
		
		public uint ID;
		public uint publicEventObjectiveGatherResourceEnumFlag;
		public uint creature2IdContainer;
		public uint creature2IdResource;
		public uint spell4IdResource;
		public uint creature2IdStolenResource;
		public uint spell4IdStolenResource;
		public uint publicEventObjectiveGatherResourceIdOpposing;
	}
}
