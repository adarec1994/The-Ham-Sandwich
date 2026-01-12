namespace ProjectWS.Engine.Database.Definitions
{
	public class StoreDisplayInfo : TblRecord
	{
		public override string GetFileName() => "StoreDisplayInfo";
		public override uint GetID() => this.ID;
		
		public uint ID;
		public uint displayType;
		public uint displayValue;
		public uint modelCameraId;
		public uint localizedTextIdName;
	}
}
