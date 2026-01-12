namespace ProjectWS.Engine.Database.Definitions
{
	public class PublicEventTeam : TblRecord
	{
		public override string GetFileName() => "PublicEventTeam";
		public override uint GetID() => this.ID;
		
		public uint ID;
		public uint localizedTextIdName;
	}
}
