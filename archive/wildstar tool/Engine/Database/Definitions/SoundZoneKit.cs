namespace ProjectWS.Engine.Database.Definitions
{
	public class SoundZoneKit : TblRecord
	{
		public override string GetFileName() => "SoundZoneKit";
		public override uint GetID() => this.ID;
		
		public uint ID;
		public uint soundZoneKitIdParent;
		public uint worldZoneId;
		public uint inheritFlags;
		public uint propertyFlags;
		public uint soundMusicSetId;
		public uint soundEventIdIntro;
		public float introReplayWait;
		public uint soundEventIdMusicMood;
		public uint soundEventIdAmbientDay;
		public uint soundEventIdAmbientNight;
		public uint soundEventIdAmbientUnderwater;
		public uint soundEventIdAmbientStop;
		public uint soundEventIdAmbientPreStopOverride;
		public uint soundEnvironmentId00;
		public uint soundEnvironmentId01;
		public float environmentDry;
		public float environmentWet00;
		public float environmentWet01;
	}
}
