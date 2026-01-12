namespace ProjectWS.Engine.Database.Definitions
{
	public class Spell4Telegraph : TblRecord
	{
		public override string GetFileName() => "Spell4Telegraph";
		public override uint GetID() => this.ID;
		
		public uint ID;
		public uint spell4Id;
		public uint telegraphDamageId;
	}
}
