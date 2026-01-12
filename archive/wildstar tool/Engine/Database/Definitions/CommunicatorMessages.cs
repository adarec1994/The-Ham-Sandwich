namespace ProjectWS.Engine.Database.Definitions
{
	public class CommunicatorMessages : TblRecord
	{
		public override string GetFileName() => "CommunicatorMessages";
		public override uint GetID() => this.ID;
		
		public uint ID;
		public uint localizedTextIdMessage;
		public uint delay;
		public uint flags;
		public uint creatureId;
		public uint worldId;
		public uint worldZoneId;
		public uint minLevel;
		public uint maxLevel;
		public uint quest00;
		public uint quest01;
		public uint quest02;
		public uint state00;
		public uint state01;
		public uint state02;
		public uint factionId;
		public uint classId;
		public uint raceId;
		public uint factionIdReputation;
		public uint reputationMin;
		public uint reputationMax;
		public uint questIdDelivered;
		public uint prerequisiteId;
		public uint displayDuration;
		public uint communicatorMessagesIdNext;
		public uint communicatorPortraitPlacementEnum;
		public uint communicatorOverlayEnum;
		public uint communicatorBackgroundEnum;
	}
}
