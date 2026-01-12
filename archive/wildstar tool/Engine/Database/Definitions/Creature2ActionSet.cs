namespace ProjectWS.Engine.Database.Definitions
{
	public class Creature2ActionSet : TblRecord
	{
		public override string GetFileName() => "Creature2ActionSet";
		public override uint GetID() => this.ID;
		
		public uint ID;
		public string description;
		public uint prerequisiteId;
	}
}
