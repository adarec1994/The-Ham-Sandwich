namespace ProjectWS.Engine.Database.Definitions
{
	public class Spell4AoeTargetConstraints : TblRecord
	{
		public override string GetFileName() => "Spell4AoeTargetConstraints";
		public override uint GetID() => this.ID;
		
		public uint ID;
		public float angle;
		public uint targetCount;
		public float minRange;
		public float maxRange;
		public uint targetSelection;
	}
}
