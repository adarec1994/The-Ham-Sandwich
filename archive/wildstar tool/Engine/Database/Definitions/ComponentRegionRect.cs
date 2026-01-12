namespace ProjectWS.Engine.Database.Definitions
{
	public class ComponentRegionRect : TblRecord
	{
		public override string GetFileName() => "ComponentRegionRect";
		public override uint GetID() => this.ID;
		
		public uint ID;
		public uint regionId;
		public uint rectMinX;
		public uint rectMinY;
		public uint rectLimX;
		public uint rectLimY;
		public uint componentLayoutId;
	}
}
