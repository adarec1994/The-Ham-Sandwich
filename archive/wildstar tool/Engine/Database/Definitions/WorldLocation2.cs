namespace ProjectWS.Engine.Database.Definitions
{
	public class WorldLocation2 : TblRecord
	{
		public override string GetFileName() => "WorldLocation2";
		public override uint GetID() => this.ID;
		
		public uint ID;
		public float radius;
		public float maxVerticalDistance;
		public float position0;
		public float position1;
		public float position2;
		public float facing0;
		public float facing1;
		public float facing2;
		public float facing3;
		public uint worldId;
		public uint worldZoneId;
		public uint phases;
	}
}
