namespace ProjectWS.Engine.Database.Definitions
{
	public class PathEpisode : TblRecord
	{
		public override string GetFileName() => "PathEpisode";
		public override uint GetID() => this.ID;
		
		public uint ID;
		public uint localizedTextIdName;
		public uint localizedTextIdSummary;
		public uint worldId;
		public uint worldZoneId;
		public uint pathTypeEnum;
	}
}
