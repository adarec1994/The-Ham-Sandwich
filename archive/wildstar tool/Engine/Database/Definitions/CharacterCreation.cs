namespace ProjectWS.Engine.Database.Definitions
{
	public class CharacterCreation : TblRecord
	{
		public override string GetFileName() => "CharacterCreation";
		public override uint GetID() => this.ID;
		
		public uint ID;
		public uint classId;
		public uint raceId;
		public uint sex;
		public uint factionId;
		public uint costumeOnly;
		public uint itemId0;
		public uint itemId01;
		public uint itemId02;
		public uint itemId03;
		public uint itemId04;
		public uint itemId05;
		public uint itemId06;
		public uint itemId07;
		public uint itemId08;
		public uint itemId09;
		public uint itemId010;
		public uint itemId011;
		public uint itemId012;
		public uint itemId013;
		public uint itemId014;
		public uint itemId015;
		public uint enabled;
		public uint characterCreationStartEnum;
		public uint xp;
		public uint accountCurrencyTypeIdCost;
		public uint accountCurrencyAmountCost;
		public uint entitlementIdRequired;
	}
}
