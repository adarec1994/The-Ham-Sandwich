namespace ProjectWS.Engine.Database.Definitions
{
	public class WindSpawn : TblRecord
	{
		public override string GetFileName() => "WindSpawn";
		public override uint GetID() => this.ID;
		
		public uint ID;
		public uint intervalMin;
		public uint intervalMax;
		public float directionMin;
		public float directionMax;
	}
}
