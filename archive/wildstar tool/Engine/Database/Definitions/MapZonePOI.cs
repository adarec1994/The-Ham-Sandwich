namespace ProjectWS.Engine.Database.Definitions
{
	public class MapZonePOI : TblRecord
	{
		public override string GetFileName() => "MapZonePOI";
		public override uint GetID() => this.ID;
		
		public uint ID;
		public uint mapZoneId;
		public uint pos0;
		public uint pos1;
		public uint pos2;
		public uint localizedTextId;
		public uint mapZoneSpriteId;
	}
}
