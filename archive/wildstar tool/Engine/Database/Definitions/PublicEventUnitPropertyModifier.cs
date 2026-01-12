namespace ProjectWS.Engine.Database.Definitions
{
	public class PublicEventUnitPropertyModifier : TblRecord
	{
		public override string GetFileName() => "PublicEventUnitPropertyModifier";
		public override uint GetID() => this.ID;
		
		public uint ID;
		public uint publicEventId;
		public uint unitProperty2Id;
		public float scalar;
	}
}
