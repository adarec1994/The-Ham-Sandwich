namespace ProjectWS.Engine.Database.Definitions
{
	public class Spell4CastResult : TblRecord
	{
		public override string GetFileName() => "Spell4CastResult";
		public override uint GetID() => this.ID;
		
		public uint ID;
		public string enumName;
		public uint combatMessageTypeEnum;
		public uint localizedTextIdDisplayText;
		public uint soundEventId;
	}
}
