namespace ProjectWS.Engine.Database.Definitions
{
	public class TradeskillMaterial : TblRecord
	{
		public override string GetFileName() => "TradeskillMaterial";
		public override uint GetID() => this.ID;
		
		public uint ID;
		public uint item2IdStatRevolution;
		public uint item2Id;
		public uint displayIndex;
		public uint tradeskillMaterialCategoryId;
	}
}
