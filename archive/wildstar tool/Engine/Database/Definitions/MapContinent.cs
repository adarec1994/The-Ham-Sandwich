namespace ProjectWS.Engine.Database.Definitions
{
	public class MapContinent : TblRecord
	{
		public override string GetFileName() => "MapContinent";
		public override uint GetID() => this.ID;
		
		public uint ID;
		public uint localizedTextIdName;
		public string assetPath;				// Path in "Map" folder that contains the area files and minimap files
		public string imagePath;				// Path in "UI/Maps" folder that contains a stylized map
		public uint imageWidth;
		public uint imageHeight;
		public uint imageOffsetX;
		public uint imageOffsetY;
		public uint hexMinX;
		public uint hexMinY;
		public uint hexLimX;
		public uint hexLimY;
		public uint flags;
	}
}
