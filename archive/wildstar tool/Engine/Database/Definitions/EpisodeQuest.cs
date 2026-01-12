namespace ProjectWS.Engine.Database.Definitions
{
	public class EpisodeQuest : TblRecord
	{
		public override string GetFileName() => "EpisodeQuest";
		public override uint GetID() => this.ID;
		
		public uint ID;
		public uint episodeId;
		public uint questId;
		public uint orderIdx;
		public uint flags;
	}
}
