namespace ProjectWS.Engine.Database.Definitions
{
	public class TrackingSlot : TblRecord
	{
		public override string GetFileName() => "TrackingSlot";
		public override uint GetID() => this.ID;
		
		public uint ID;
		public uint localizedTextIdLabel;
		public string iconPath;
		public uint publicEventObjectiveId;
	}
}
