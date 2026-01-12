namespace ProjectWS.Engine.Database.Definitions
{
	public class PathSettlerImprovement : TblRecord
	{
		public override string GetFileName() => "PathSettlerImprovement";
		public override uint GetID() => this.ID;
		
		public uint ID;
		public uint countResource00;
		public uint countResource01;
		public uint countResource02;
		public uint countRecontributionResource00;
		public uint countRecontributionResource01;
		public uint countRecontributionResource02;
		public uint spell4IdDisplay;
	}
}
