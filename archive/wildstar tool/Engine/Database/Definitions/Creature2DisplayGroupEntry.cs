namespace ProjectWS.Engine.Database.Definitions
{
	public class Creature2DisplayGroupEntry : TblRecord
	{
		public override string GetFileName() => "Creature2DisplayGroupEntry";
		public override uint GetID() => this.ID;
		
		public uint ID;
		public uint creature2DisplayGroupId;
		public uint creature2DisplayInfoId;
		public uint weight;
	}
}
