namespace ProjectWS.Engine.Database.Definitions
{
	public class SoundReplaceDescription : TblRecord
	{
		public override string GetFileName() => "SoundReplaceDescription";
		public override uint GetID() => this.ID;
		
		public uint ID;
	}
}
