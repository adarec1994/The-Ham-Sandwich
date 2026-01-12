namespace ProjectWS.Engine.Database.Definitions
{
	public class CharacterCustomizationLabel : TblRecord
	{
		public override string GetFileName() => "CharacterCustomizationLabel";
		public override uint GetID() => this.ID;
		
		public uint ID;
		public uint localizedTextId;
		public uint faction2Id;
		public uint displayIndex;
	}
}
