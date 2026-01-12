namespace ProjectWS.Engine.Database.Definitions
{
	public class MaterialData : TblRecord
	{
		public override string GetFileName() => "MaterialData";
		public override uint GetID() => this.ID;
		
		public uint ID;
		public uint row;
		public uint materialTypeId;
	}
}
