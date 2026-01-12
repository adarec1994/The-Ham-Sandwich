namespace ProjectWS.Engine.Database.Definitions
{
	public class Spell4TargetMechanics : TblRecord
	{
		public override string GetFileName() => "Spell4TargetMechanics";
		public override uint GetID() => this.ID;
		
		public uint ID;
		public uint targetType;
		public uint flags;
	}
}
