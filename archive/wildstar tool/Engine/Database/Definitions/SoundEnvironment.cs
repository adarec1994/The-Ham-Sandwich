namespace ProjectWS.Engine.Database.Definitions
{
	public class SoundEnvironment : TblRecord
	{
		public override string GetFileName() => "SoundEnvironment";
		public override uint GetID() => this.ID;
		
		public uint ID;
		public uint hash;
	}
}
