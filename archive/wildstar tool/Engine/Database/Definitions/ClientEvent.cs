namespace ProjectWS.Engine.Database.Definitions
{
	public class ClientEvent : TblRecord
	{
		public override string GetFileName() => "ClientEvent";
		public override uint GetID() => this.ID;
		
		public uint ID;
		public string description;
		public uint worldId;
		public uint eventTypeEnum;
		public uint eventData;
		public uint prerequisiteId;
		public uint priority;
		public uint delayMS;
		public uint clientEventActionId;
	}
}
