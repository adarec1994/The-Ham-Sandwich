namespace ProjectWS.Engine.Database.Definitions
{
	public class QuestGroup : TblRecord
	{
		public override string GetFileName() => "QuestGroup";
		public override uint GetID() => this.ID;
		
		public uint ID;
		public uint localizedTextId;
	}
}
