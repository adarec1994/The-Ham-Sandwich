namespace ProjectWS.Engine.Database.Definitions
{
	public class Creature2OverrideProperties : TblRecord
	{
		public override string GetFileName() => "Creature2OverrideProperties";
		public override uint GetID() => this.ID;
		
		public uint ID;
		public uint creature2Id;
		public uint unitPropertyIndex;
		public float unitPropertyValue;
	}
}
