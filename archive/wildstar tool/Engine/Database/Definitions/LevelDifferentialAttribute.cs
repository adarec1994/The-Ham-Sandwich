namespace ProjectWS.Engine.Database.Definitions
{
	public class LevelDifferentialAttribute : TblRecord
	{
		public override string GetFileName() => "LevelDifferentialAttribute";
		public override uint GetID() => this.ID;
		
		public uint ID;
		public uint localizedTextIdDescription;
		public uint levelDifferentialValue;
		public float questXpMultiplier;
	}
}
