namespace ProjectWS.Engine.Database.Definitions
{
	public class CharacterTitleCategory : TblRecord
	{
		public override string GetFileName() => "CharacterTitleCategory";
		public override uint GetID() => this.ID;
		
		public uint ID;
		public uint localizedTextId;
	}
}
