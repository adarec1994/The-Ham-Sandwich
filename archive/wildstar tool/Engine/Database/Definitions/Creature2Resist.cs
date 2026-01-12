namespace ProjectWS.Engine.Database.Definitions
{
	public class Creature2Resist : TblRecord
	{
		public override string GetFileName() => "Creature2Resist";
		public override uint GetID() => this.ID;
		
		public uint ID;
		public float resistPhysicalMultiplier;
		public float resistTechMultiplier;
		public float resistMagicMultiplier;
	}
}
