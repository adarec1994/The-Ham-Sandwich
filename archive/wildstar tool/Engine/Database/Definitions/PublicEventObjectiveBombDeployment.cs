namespace ProjectWS.Engine.Database.Definitions
{
	public class PublicEventObjectiveBombDeployment : TblRecord
	{
		public override string GetFileName() => "PublicEventObjectiveBombDeployment";
		public override uint GetID() => this.ID;
		
		public uint ID;
		public uint creature2IdBomb;
		public uint spell4IdCarrying;
	}
}
