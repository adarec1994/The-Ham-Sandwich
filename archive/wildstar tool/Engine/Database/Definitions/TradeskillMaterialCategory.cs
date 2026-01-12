namespace ProjectWS.Engine.Database.Definitions
{
	public class TradeskillMaterialCategory : TblRecord
	{
		public override string GetFileName() => "TradeskillMaterialCategory";
		public override uint GetID() => this.ID;
		
		public uint ID;
		public uint localizedTextIdName;
	}
}
