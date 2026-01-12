namespace ProjectWS.Engine.Database.Definitions
{
	public class MatchingGameMap : TblRecord
	{
		public override string GetFileName() => "MatchingGameMap";
		public override uint GetID() => this.ID;
		
		public uint ID;
		public uint matchingGameMapEnumFlags;
		public uint localizedTextIdName;
		public uint localizedTextIdDescription;
		public uint matchingGameTypeId;
		public uint worldId;
		public uint recommendedItemLevel;
		public uint achievementCategoryId;
		public uint prerequisiteId;
	}
}
