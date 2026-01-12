namespace ProjectWS.Engine.Database.Definitions
{
	public class Spell4Conditions : TblRecord
	{
		public override string GetFileName() => "Spell4Conditions";
		public override uint GetID() => this.ID;
		
		public uint ID;
		public uint conditionMask;
		public uint conditionValue;
	}
}
