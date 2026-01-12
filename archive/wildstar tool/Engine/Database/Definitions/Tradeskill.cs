namespace ProjectWS.Engine.Database.Definitions
{
	public class Tradeskill : TblRecord
	{
		public override string GetFileName() => "Tradeskill";
		public override uint GetID() => this.ID;
		
		public uint ID;
		public uint localizedTextIdName;
		public uint localizedTextIdDescription;
		public uint flags;
		public uint tutorialId;
		public uint achievementCategoryId;
		public uint maxAdditives;
		public uint localizedTextIdAxisName00;
		public uint localizedTextIdAxisName01;
		public uint localizedTextIdAxisName02;
		public uint localizedTextIdAxisName03;
	}
}
