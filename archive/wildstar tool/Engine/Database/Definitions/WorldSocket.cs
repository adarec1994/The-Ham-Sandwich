namespace ProjectWS.Engine.Database.Definitions
{
	public class WorldSocket : TblRecord
	{
		public override string GetFileName() => "WorldSocket";
		public override uint GetID() => this.ID;
		
		public uint ID;
		public uint worldId;
		public uint bounds0;
		public uint bounds1;
		public uint bounds2;
		public uint bounds3;
		public uint averageHeight;
	}
}
