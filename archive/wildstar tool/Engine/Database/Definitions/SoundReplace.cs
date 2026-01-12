namespace ProjectWS.Engine.Database.Definitions
{
	public class SoundReplace : TblRecord
	{
		public override string GetFileName() => "SoundReplace";
		public override uint GetID() => this.ID;
		
		public uint ID;
		public uint soundReplaceDescriptionId;
		public uint soundEventIdOld;
		public uint soundEventIdNew;
	}
}
