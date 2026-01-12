namespace ProjectWS.Engine.Database.Definitions
{
	public class ChallengeTier : TblRecord
	{
		public override string GetFileName() => "ChallengeTier";
		public override uint GetID() => this.ID;
		
		public uint ID;
		public uint count;
	}
}
