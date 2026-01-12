namespace ProjectWS.Engine.Database.Definitions
{
	public class ColorShift : TblRecord
	{
		public override string GetFileName() => "ColorShift";
		public override uint GetID() => this.ID;
		
		public uint ID;
		public string texturePath;
		public uint localizedTextId;
		public string previewSwatchIcon;
	}
}
