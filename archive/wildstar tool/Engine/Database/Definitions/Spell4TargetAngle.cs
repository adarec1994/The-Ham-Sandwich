namespace ProjectWS.Engine.Database.Definitions
{
	public class Spell4TargetAngle : TblRecord
	{
		public override string GetFileName() => "Spell4TargetAngle";
		public override uint GetID() => this.ID;
		
		public uint ID;
		public float targetAngle;
	}
}
