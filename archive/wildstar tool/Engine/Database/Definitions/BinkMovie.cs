namespace ProjectWS.Engine.Database.Definitions
{
	public class BinkMovie : TblRecord
	{
		public override string GetFileName() => "BinkMovie";
		public override uint GetID() => this.ID;
		
		public uint ID;
		public string binkMovieAssetPath;
		public uint flags;
	}
}
