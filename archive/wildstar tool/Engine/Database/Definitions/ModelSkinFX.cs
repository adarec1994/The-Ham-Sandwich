namespace ProjectWS.Engine.Database.Definitions
{
	public class ModelSkinFX : TblRecord
	{
		public override string GetFileName() => "ModelSkinFX";
		public override uint GetID() => this.ID;
		
		public uint ID;
		public string suffix;
	}
}
