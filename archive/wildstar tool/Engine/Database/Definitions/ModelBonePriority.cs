namespace ProjectWS.Engine.Database.Definitions
{
	public class ModelBonePriority : TblRecord
	{
		public override string GetFileName() => "ModelBonePriority";
		public override uint GetID() => this.ID;
		
		public uint ID;
		public uint BoneID;
		public uint BoneSetID;
		public uint Priority;
	}
}
