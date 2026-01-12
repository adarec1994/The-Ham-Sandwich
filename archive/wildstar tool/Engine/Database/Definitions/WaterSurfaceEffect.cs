namespace ProjectWS.Engine.Database.Definitions
{
	public class WaterSurfaceEffect : TblRecord
	{
		public override string GetFileName() => "WaterSurfaceEffect";
		public override uint GetID() => this.ID;
		
		public uint ID;
		public uint emissionDelay;
		public uint worldWaterWakeIdStillWater0;
		public uint worldWaterWakeIdStillWater1;
		public uint visualEffectIdParticle0;
		public uint visualEffectIdParticle1;
		public uint particleFlags0;
		public uint particleFlags1;
	}
}
