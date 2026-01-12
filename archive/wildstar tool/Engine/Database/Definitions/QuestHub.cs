namespace ProjectWS.Engine.Database.Definitions
{
	public class QuestHub : TblRecord
	{
		public override string GetFileName() => "QuestHub";
		public override uint GetID() => this.ID;
		
		public uint ID;
		public uint worldLocation2Id;
		public uint localizedTextIdName;
	}
}
