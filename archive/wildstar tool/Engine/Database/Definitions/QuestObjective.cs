namespace ProjectWS.Engine.Database.Definitions
{
	public class QuestObjective : TblRecord
	{
		public override string GetFileName() => "QuestObjective";
		public override uint GetID() => this.ID;
		
		public uint ID;
		public uint type;
		public uint flags;
		public uint data;
		public uint count;
		public uint localizedTextIdFull;
		public uint worldLocationsIdIndicator00;
		public uint worldLocationsIdIndicator01;
		public uint worldLocationsIdIndicator02;
		public uint worldLocationsIdIndicator03;
		public uint maxTimeAllowedMS;
		public uint localizedTextIdShort;
		public uint targetGroupIdRewardPane;
		public uint questDirectionId;
	}
}
