namespace ProjectWS.Engine.Database.Definitions
{
	public class PublicEventCustomStat : TblRecord
	{
		public override string GetFileName() => "PublicEventCustomStat";
		public override uint GetID() => this.ID;
		
		public uint ID;
		public uint publicEventTypeEnum;
		public uint publicEventId;
		public uint statIndex;
		public uint localizedTextIdStatName;
		public string iconPath;
	}
}
