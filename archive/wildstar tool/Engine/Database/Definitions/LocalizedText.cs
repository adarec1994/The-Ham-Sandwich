namespace ProjectWS.Engine.Database.Definitions
{
	public class LocalizedText : TblRecord
	{
		public override string GetFileName() => "LocalizedText";
		public override uint GetID() => this.ID;
		
		public uint ID;
		public uint soundEventId;
		public uint soundEventIdFemale;
		public uint version;
		public uint unitVoiceTypeId;
		public uint stringContextEnum;
		public uint soundAvailabilityFlagFemale;
	}
}
