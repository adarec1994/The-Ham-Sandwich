namespace ProjectWS.Engine.Database.Definitions
{
	public class MaterialType : TblRecord
	{
		public override string GetFileName() => "MaterialType";
		public override uint GetID() => this.ID;
		
		public uint ID;
		public uint MaterialFlags;
	}
}
