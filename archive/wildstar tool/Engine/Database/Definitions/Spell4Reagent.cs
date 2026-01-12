namespace ProjectWS.Engine.Database.Definitions
{
	public class Spell4Reagent : TblRecord
	{
		public override string GetFileName() => "Spell4Reagent";
		public override uint GetID() => this.ID;
		
		public uint ID;
		public uint reagentType;
		public uint reagentTypeObjectId;
		public uint reagentCount;
		public uint consumeReagent;
	}
}
