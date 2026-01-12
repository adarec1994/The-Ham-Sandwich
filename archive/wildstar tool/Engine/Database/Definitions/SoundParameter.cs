namespace ProjectWS.Engine.Database.Definitions
{
	public class SoundParameter : TblRecord
	{
		public override string GetFileName() => "SoundParameter";
		public override uint GetID() => this.ID;
		
		public uint ID;
		public uint hash;
	}
}
