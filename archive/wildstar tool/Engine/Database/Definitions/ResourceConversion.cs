namespace ProjectWS.Engine.Database.Definitions
{
	public class ResourceConversion : TblRecord
	{
		public override string GetFileName() => "ResourceConversion";
		public override uint GetID() => this.ID;
		
		public uint ID;
		public uint resourceConversionTypeEnum;
		public uint sourceId;
		public uint sourceCount;
		public uint targetId;
		public uint targetCount;
		public uint surchargeId;
		public uint surchargeCount;
		public uint flags;
	}
}
