namespace ProjectWS.Engine.Database.Definitions
{
	public class PublicEventStatDisplay : TblRecord
	{
		public override string GetFileName() => "PublicEventStatDisplay";
		public override uint GetID() => this.ID;
		
		public uint ID;
		public uint publicEventTypeEnum;
		public uint publicEventId;
		public uint flags;
	}
}
