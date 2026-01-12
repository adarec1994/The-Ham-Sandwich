namespace ProjectWS.Engine.Database.Definitions
{
	public class PathSettlerHub : TblRecord
	{
		public override string GetFileName() => "PathSettlerHub";
		public override uint GetID() => this.ID;
		
		public uint ID;
		public uint maxAvenueEconomy;
		public uint maxAvenueSecurity;
		public uint maxAvenueQualityOfLife;
		public uint localizedTextIdEconomy;
		public uint localizedTextIdSecurity;
		public uint localizedTextIdQualityOfLife;
		public uint worldZoneId;
		public uint missionCount;
		public uint flags;
		public uint item2IdResource00;
		public uint item2IdResource01;
		public uint item2IdResource02;
		public uint publicEventObjectiveIdResource00;
		public uint publicEventObjectiveIdResource01;
		public uint publicEventObjectiveIdResource02;
		public uint worldLocation2IdMapResource00Loc00;
		public uint worldLocation2IdMapResource00Loc01;
		public uint worldLocation2IdMapResource00Loc02;
		public uint worldLocation2IdMapResource00Loc03;
		public uint worldLocation2IdMapResource01Loc00;
		public uint worldLocation2IdMapResource01Loc01;
		public uint worldLocation2IdMapResource01Loc02;
		public uint worldLocation2IdMapResource01Loc03;
		public uint worldLocation2IdMapResource02Loc00;
		public uint worldLocation2IdMapResource02Loc01;
		public uint worldLocation2IdMapResource02Loc02;
		public uint worldLocation2IdMapResource02Loc03;
		public uint localizedTextIdRewardNotify;
	}
}
