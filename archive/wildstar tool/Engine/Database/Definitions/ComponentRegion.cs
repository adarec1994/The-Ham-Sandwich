namespace ProjectWS.Engine.Database.Definitions
{
	public class ComponentRegion : TblRecord
	{
		public override string GetFileName() => "ComponentRegion";
		public override uint GetID() => this.ID;
		
		public uint ID;
		public string EnumName;
		public uint componentMap;
	}
}
