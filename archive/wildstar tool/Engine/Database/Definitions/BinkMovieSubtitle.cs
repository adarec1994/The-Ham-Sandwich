namespace ProjectWS.Engine.Database.Definitions
{
	public class BinkMovieSubtitle : TblRecord
	{
		public override string GetFileName() => "BinkMovieSubtitle";
		public override uint GetID() => this.ID;
		
		public uint ID;
		public uint binkMovieId;
		public uint delayMs;
		public uint localizedTextIdDisplayText;
	}
}
