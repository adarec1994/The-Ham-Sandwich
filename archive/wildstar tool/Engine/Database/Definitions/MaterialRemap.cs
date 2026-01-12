namespace ProjectWS.Engine.Database.Definitions
{
	public class MaterialRemap : TblRecord
	{
		public override string GetFileName() => "MaterialRemap";
		public override uint GetID() => this.ID;
		
		public uint ID;
		public uint materialDataRow;
		public uint materialSetId;
		public uint materialDataRowRemap;
	}
}
