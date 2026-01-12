namespace ProjectWS.Engine.Database.Definitions
{
	public class ChatChannel : TblRecord
	{
		public override string GetFileName() => "ChatChannel";
		public override uint GetID() => this.ID;
		
		public uint ID;
		public string enumName;
		public string universalCommand00;
		public string universalCommand01;
		public uint localizedTextIdName;
		public uint localizedTextIdCommand;
		public uint localizedTextIdAbbreviation;
		public uint localizedTextIdAlternate00;
		public uint localizedTextIdAlternate01;
		public uint flags;
	}
}
