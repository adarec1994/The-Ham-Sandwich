namespace ProjectWS.Engine.Database.Definitions
{
	public class ModelBone : TblRecord
	{
		public override string GetFileName() => "ModelBone";
		public override uint GetID() => this.ID;
		
		public uint ID;
		public string XSIName;
	}
}
