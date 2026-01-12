namespace ProjectWS.Engine.Database.Definitions
{
	public class MapZoneSprite : TblRecord
	{
		public override string GetFileName() => "MapZoneSprite";
		public override uint GetID() => this.ID;
		
		public uint ID;
		public string spriteName;
	}
}
