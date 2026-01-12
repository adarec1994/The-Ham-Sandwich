namespace ProjectWS.Engine.Database.Definitions
{
	public class CharacterCustomization : TblRecord
	{
		public override string GetFileName() => "CharacterCustomization";
		public override uint GetID() => this.ID;
		
		public uint ID;
		public uint raceId;
		public uint gender;
		public uint itemSlotId;
		public uint itemDisplayId;
		public uint flags;
		public uint characterCustomizationLabelId00;
		public uint characterCustomizationLabelId01;
		public uint value00;
		public uint value01;
	}
}
