namespace ProjectWS.Engine.Database.Definitions
{
	public class WorldWaterEnvironment : TblRecord
	{
		public override string GetFileName() => "WorldWaterEnvironment";
		public override uint GetID() => this.ID;
		
		public uint ID;
		public string LandMapPath;
	}
}
