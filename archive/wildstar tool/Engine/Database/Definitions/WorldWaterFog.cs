namespace ProjectWS.Engine.Database.Definitions
{
	public class WorldWaterFog : TblRecord
	{
		public override string GetFileName() => "WorldWaterFog";
		public override uint GetID() => this.ID;
		
		public uint ID;
		public float fogStart;
		public float fogEnd;
		public float fogStartUW;
		public float fogEndUW;
		public float modStart;
		public float modEnd;
		public float modStartUW;
		public float modEndUW;
		public uint skyColorIndex;
	}
}
