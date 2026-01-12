namespace ProjectWS.Engine.Database.Definitions
{
	public class LootSpell : TblRecord
	{
		public override string GetFileName() => "LootSpell";
		public override uint GetID() => this.ID;
		
		public uint ID;
		public uint lootSpellTypeEnum;
		public uint lootSpellPickupEnumFlags;
		public uint creature2Id;
		public string buttonIcon;
		public uint localizedTextIdName;
		public uint localizedTextIdDescription;
		public uint visualEffectId;
		public uint data;
		public uint dataValue;
	}
}
