namespace ProjectWS.Engine.Database.Definitions
{
	public class PublicEventVote : TblRecord
	{
		public override string GetFileName() => "PublicEventVote";
		public override uint GetID() => this.ID;
		
		public uint ID;
		public uint localizedTextIdTitle;
		public uint localizedTextIdDescription;
		public uint localizedTextIdOption00;
		public uint localizedTextIdOption01;
		public uint localizedTextIdOption02;
		public uint localizedTextIdOption03;
		public uint localizedTextIdOption04;
		public uint localizedTextIdLabel00;
		public uint localizedTextIdLabel01;
		public uint localizedTextIdLabel02;
		public uint localizedTextIdLabel03;
		public uint localizedTextIdLabel04;
		public uint durationMS;
		public string assetPathSprite;
	}
}
