namespace ProjectWS.Engine.Database.Definitions
{
	public class CinematicRace : TblRecord
	{
		public override string GetFileName() => "CinematicRace";
		public override uint GetID() => this.ID;
		
		public uint ID;
		public uint cinematicId;
		public uint raceId;
		public string maleAssetPath;
		public string femaleAssetPath;
	}
}
