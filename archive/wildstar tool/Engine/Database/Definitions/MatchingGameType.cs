namespace ProjectWS.Engine.Database.Definitions
{
	public class MatchingGameType : TblRecord
	{
		public override string GetFileName() => "MatchingGameType";
		public override uint GetID() => this.ID;
		
		public uint ID;
		public uint localizedTextIdName;
		public uint localizedTextIdDescription;
		public uint matchTypeEnum;
		public uint matchingGameTypeEnumFlags;
		public uint teamSize;
		public uint minLevel;
		public uint maxLevel;
		public uint preparationTimeMS;
		public uint matchTimeMS;
		public uint matchingRulesEnum;
		public uint matchingRulesData00;
		public uint matchingRulesData01;
		public uint targetItemLevel;
	}
}
