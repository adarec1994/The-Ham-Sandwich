namespace ProjectWS.Engine.Database.Definitions
{
	public class MissileRevolverTrack : TblRecord
	{
		public override string GetFileName() => "MissileRevolverTrack";
		public override uint GetID() => this.ID;
		
		public uint ID;
		public float radius;
		public float speed;
		public float speedMultiplier;
		public float scaleMultiplier;
		public uint modelAttachmentIdHeight;
	}
}
