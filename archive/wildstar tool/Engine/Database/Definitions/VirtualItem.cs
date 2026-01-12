namespace ProjectWS.Engine.Database.Definitions
{
	public class VirtualItem : TblRecord
	{
		public override string GetFileName() => "VirtualItem";
		public override uint GetID() => this.ID;
		
		public uint ID;
		public string buttonIcon;
		public uint item2TypeId;
		public uint localizedTextIdName;
		public uint localizedTextIdFlavor;
		public uint itemQualityId;
	}
}
