namespace ProjectWS.Engine.Database.Definitions
{
	public class SoundContext : TblRecord
	{
		public override string GetFileName() => "SoundContext";
		public override uint GetID() => this.ID;
		
		public uint ID;
		public uint eventId;
		public uint type;
	}
}
