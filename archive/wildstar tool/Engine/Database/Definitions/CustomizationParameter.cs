namespace ProjectWS.Engine.Database.Definitions
{
	public class CustomizationParameter : TblRecord
	{
		public override string GetFileName() => "CustomizationParameter";
		public override uint GetID() => this.ID;
		
		public uint ID;
		public uint localizedTextId;
		public uint sclX;
		public uint sclY;
		public uint sclZ;
		public uint rotX;
		public uint rotY;
		public uint rotZ;
		public uint posX;
		public uint posY;
		public uint posZ;
	}
}
