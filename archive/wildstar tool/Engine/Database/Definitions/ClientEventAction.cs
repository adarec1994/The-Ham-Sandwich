namespace ProjectWS.Engine.Database.Definitions
{
	public class ClientEventAction : TblRecord
	{
		public override string GetFileName() => "ClientEventAction";
		public override uint GetID() => this.ID;
		
		public uint ID;
		public uint actionTypeEnum;
		public uint actionData00;
		public uint actionData01;
		public uint localizedTextIdSubZoneName;
	}
}
