namespace ProjectWS.Engine.Database.Definitions
{
	public class SoundDirectionalAmbience : TblRecord
	{
		public override string GetFileName() => "SoundDirectionalAmbience";
		public override uint GetID() => this.ID;
		
		public uint ID;
		public uint soundEventIdOutsideStart;
		public uint soundEventIdOutsideStop;
	}
}
