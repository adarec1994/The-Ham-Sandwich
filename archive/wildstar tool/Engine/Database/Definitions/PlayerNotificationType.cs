namespace ProjectWS.Engine.Database.Definitions
{
	public class PlayerNotificationType : TblRecord
	{
		public override string GetFileName() => "PlayerNotificationType";
		public override uint GetID() => this.ID;
		
		public uint ID;
		public uint priority;
		public uint lifetimeMs;
	}
}
