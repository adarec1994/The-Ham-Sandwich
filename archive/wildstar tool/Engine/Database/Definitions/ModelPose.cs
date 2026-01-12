namespace ProjectWS.Engine.Database.Definitions
{
	public class ModelPose : TblRecord
	{
		public override string GetFileName() => "ModelPose";
		public override uint GetID() => this.ID;
		
		public uint ID;
		public uint sequenceId;
		public string description;
		public uint modelPoseIdBase;
		public uint modelPoseTypeEnum;
	}
}
