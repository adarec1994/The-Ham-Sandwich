namespace ProjectWS.Engine.Database.Definitions
{
	public class RandomPlayerName : TblRecord
	{
		public override string GetFileName() => "RandomPlayerName";
		public override uint GetID() => this.ID;
		
		public uint ID;
		public string nameFragment;
		public uint nameFragmentTypeEnum;
		public uint raceId;
		public uint gender;
		public uint faction2Id;
		public uint languageFlags;
	}
}
