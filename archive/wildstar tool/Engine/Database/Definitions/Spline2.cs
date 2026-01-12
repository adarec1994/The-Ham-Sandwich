namespace ProjectWS.Engine.Database.Definitions
{
	public class Spline2 : TblRecord
	{
		public override string GetFileName() => "Spline2";
		public override uint GetID() => this.ID;
		
		public uint ID;
		public uint worldId;
		public uint splineType;
	}
}
