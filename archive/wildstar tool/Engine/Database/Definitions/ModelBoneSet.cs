namespace ProjectWS.Engine.Database.Definitions
{
	public class ModelBoneSet : TblRecord
	{
		public override string GetFileName() => "ModelBoneSet";
		public override uint GetID() => this.ID;
		
		public uint ID;
	}
}
