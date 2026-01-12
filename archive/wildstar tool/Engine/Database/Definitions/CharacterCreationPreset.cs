namespace ProjectWS.Engine.Database.Definitions
{
	public class CharacterCreationPreset : TblRecord
	{
		public override string GetFileName() => "CharacterCreationPreset";
		public override uint GetID() => this.ID;
		
		public uint ID;
		public uint raceId;
		public uint faction2Id;
		public uint gender;
		public string stringPreset;
	}
}
