namespace ProjectWS.Engine.Database.Definitions
{
	public class SoundBank : TblRecord
	{
		public override string GetFileName() => "SoundBank";
		public override uint GetID() => this.ID;
		
		public uint ID;
		public string name;
	}
}
