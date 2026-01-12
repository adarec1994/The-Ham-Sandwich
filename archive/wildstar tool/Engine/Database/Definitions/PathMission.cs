namespace ProjectWS.Engine.Database.Definitions
{
	public class PathMission : TblRecord
	{
		public override string GetFileName() => "PathMission";
		public override uint GetID() => this.ID;
		
		public uint ID;
		public uint creature2IdUnlock;
		public uint pathTypeEnum;
		public uint pathMissionTypeEnum;
		public uint pathMissionDisplayTypeEnum;
		public uint objectId;
		public uint localizedTextIdName;
		public uint localizedTextIdSummary;
		public uint pathEpisodeId;
		public uint worldLocation2Id00;
		public uint worldLocation2Id01;
		public uint worldLocation2Id02;
		public uint worldLocation2Id03;
		public uint pathMissionFlags;
		public uint pathMissionFactionEnum;
		public uint prerequisiteId;
		public uint localizedTextIdCommunicator;
		public uint localizedTextIdUnlock;
		public uint localizedTextIdSoldierOrders;
		public uint creature2IdContactOverride;
		public uint questDirectionId;
	}
}
