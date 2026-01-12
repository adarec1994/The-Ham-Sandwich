namespace ProjectWS.Engine.Database.Definitions
{
	public class MiniMapMarker : TblRecord
	{
		public override string GetFileName() => "MiniMapMarker";
		public override uint GetID() => this.ID;
		
		public uint ID;
		public string luaName;
	}
}
