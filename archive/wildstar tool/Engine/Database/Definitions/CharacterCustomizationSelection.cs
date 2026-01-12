namespace ProjectWS.Engine.Database.Definitions
{
	public class CharacterCustomizationSelection : TblRecord
	{
		public override string GetFileName() => "CharacterCustomizationSelection";
		public override uint GetID() => this.ID;
		
		public uint ID;
		public uint characterCustomizationLabelId;
		public uint value;
		public ulong cost;
	}
}
