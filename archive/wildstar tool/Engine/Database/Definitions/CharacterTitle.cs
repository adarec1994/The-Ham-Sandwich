namespace ProjectWS.Engine.Database.Definitions
{
	public class CharacterTitle : TblRecord
	{
		public override string GetFileName() => "CharacterTitle";
		public override uint GetID() => this.ID;
		
		public uint ID;
		public uint characterTitleCategoryId;
		public uint localizedTextIdName;
		public uint localizedTextIdTitle;
		public uint spell4IdActivate;
		public uint lifeTimeSeconds;
		public uint playerTitleFlagsEnum;
		public uint scheduleId;
	}
}
