namespace ProjectWS.Engine.Database.Definitions
{
	public class SoundMusicSet : TblRecord
	{
		public override string GetFileName() => "SoundMusicSet";
		public override uint GetID() => this.ID;
		
		public uint ID;
		public uint soundEventIdStart;
		public uint soundEventIdStop;
		public float restartDelayMin;
		public float restartDelayMax;
		public uint flags;
	}
}
