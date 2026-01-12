namespace ProjectWS.Engine.Database.Definitions
{
	public class Faction2Relationship : TblRecord
	{
		public override string GetFileName() => "Faction2Relationship";
		public override uint GetID() => this.ID;
		
		public uint ID;
		public uint factionId0;
		public uint factionId1;
		public uint factionLevel;
	}
}
