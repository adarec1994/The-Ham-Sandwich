namespace ProjectWS.Engine.Database.Definitions
{
	public class SpellLevel : TblRecord
	{
		public override string GetFileName() => "SpellLevel";
		public override uint GetID() => this.ID;
		
		public uint ID;
		public uint classId;
		public uint characterLevel;
		public uint prerequisiteId;
		public uint spell4Id;
		public float costMultiplier;
	}
}
