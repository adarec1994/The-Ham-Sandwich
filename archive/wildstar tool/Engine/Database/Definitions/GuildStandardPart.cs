namespace ProjectWS.Engine.Database.Definitions
{
	public class GuildStandardPart : TblRecord
	{
		public override string GetFileName() => "GuildStandardPart";
		public override uint GetID() => this.ID;
		
		public uint ID;
		public uint guildStandardPartTypeEnum;
		public uint localizedTextIdName;
		public uint itemDisplayIdStandard;
	}
}
