namespace ProjectWS.Engine.Database.Definitions
{
	public class SoundImpactEvents : TblRecord
	{
		public override string GetFileName() => "SoundImpactEvents";
		public override uint GetID() => this.ID;
		
		public uint ID;
		public uint origin;
		public uint target;
		public uint qualifier;
		public uint wEvent;
	}
}
