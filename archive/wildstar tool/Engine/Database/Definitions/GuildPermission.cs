namespace ProjectWS.Engine.Database.Definitions
{
	public class GuildPermission : TblRecord
	{
		public override string GetFileName() => "GuildPermission";
		public override uint GetID() => this.ID;
		
		public uint ID;
		public uint localizedTextIdName;
		public uint localizedTextIdDescription;
		public string luaVariable;
		public uint localizedTextIdCommand;
		public uint guildTypeEnumFlags;
		public uint displayIndex;
	}
}
