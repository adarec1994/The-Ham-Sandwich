namespace ProjectWS.Engine.Database.Definitions
{
	public class HousingDecorInfo : TblRecord
	{
		public override string GetFileName() => "HousingDecorInfo";
		public override uint GetID() => this.ID;
		
		public uint ID;
		public uint housingDecorTypeId;
		public uint hookTypeId;
		public uint localizedTextIdName;
		public uint flags;
		public uint hookAssetId;
		public uint cost;
		public uint costCurrencyTypeId;
		public uint creature2IdActiveProp;
		public uint prerequisiteIdUnlock;
		public uint spell4IdInteriorBuff;
		public uint housingDecorLimitCategoryId;
		public string altPreviewAsset;
		public string altEditAsset;
		public float minScale;
		public float maxScale;
	}
}
