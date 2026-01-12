namespace ProjectWS.Engine.Database.Definitions
{
	public class GuildPerk : TblRecord
	{
		public override string GetFileName() => "GuildPerk";
		public override uint GetID() => this.ID;
		
		public uint ID;
		public uint localizedTextIdTitle;
		public uint localizedTextIdDescription;
		public string luaSprite;
		public string luaName;
		public uint purchaseInfluenceCost;
		public uint activateInfluenceCost;
		public uint spell4IdActivate;
		public uint guildPerkIdRequired00;
		public uint guildPerkIdRequired01;
		public uint guildPerkIdRequired02;
		public uint achievementIdRequired;
	}
}
