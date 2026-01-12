namespace ProjectWS.Engine.Database.Definitions
{
	public class GenericMap : TblRecord
	{
		public override string GetFileName() => "GenericMap";
		public override uint GetID() => this.ID;
		
		public uint ID;
		public uint mapZoneId;
	}
}
