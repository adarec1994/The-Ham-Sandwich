namespace ProjectWS.Engine.Database.Definitions
{
	public class ModelMesh : TblRecord
	{
		public override string GetFileName() => "ModelMesh";
		public override uint GetID() => this.ID;
		
		public uint ID;
		public string EnumName;
	}
}
