namespace ProjectWS.Engine.Database.Definitions
{
	public class HookAsset : TblRecord
	{
		public override string GetFileName() => "HookAsset";
		public override uint GetID() => this.ID;
		
		public uint ID;
		public string asset;
		public float scale;
		public float offsetX;
		public float offsetY;
		public float offsetZ;
		public float rotationX;
		public float rotationY;
		public float rotationZ;
	}
}
