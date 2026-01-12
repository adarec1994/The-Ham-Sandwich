namespace ProjectWS.Engine.Database.Definitions
{
	public class Episode : TblRecord
	{
		public override string GetFileName() => "Episode";
		public override uint GetID() => this.ID;
		
		public uint ID;
		public uint localizedTextIdName;
		public uint localizedTextIdBriefing;
		public uint localizedTextIdEndSummary;
		public uint flags;
		public uint worldZoneId;
		public uint percentToDisplay;
		public uint questHubIdExile;
		public uint questHubIdDominion;
	}
}
