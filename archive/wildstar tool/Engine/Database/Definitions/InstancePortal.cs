namespace ProjectWS.Engine.Database.Definitions
{
	public class InstancePortal : TblRecord
	{
		public override string GetFileName() => "InstancePortal";
		public override uint GetID() => this.ID;
		
		public uint ID;
		public uint localizedTextIdName;
		public uint minLevel;
		public uint maxLevel;
		public uint expectedCompletionTime;
		public uint instancePortalTypeEnum;
	}
}
