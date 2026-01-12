namespace ProjectWS.Engine.Database.Definitions
{
	public class WorldWaterType : TblRecord
	{
		public override string GetFileName() => "WorldWaterType";
		public override uint GetID() => this.ID;
		
		public uint ID;
		public uint worldWaterFogId;
		public uint SurfaceType;
		public string particleFile;
		public uint soundDirectionalAmbienceId;
	}
}
