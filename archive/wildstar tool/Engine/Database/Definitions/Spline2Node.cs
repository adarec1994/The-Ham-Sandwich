namespace ProjectWS.Engine.Database.Definitions
{
	public class Spline2Node : TblRecord
	{
		public override string GetFileName() => "Spline2Node";
		public override uint GetID() => this.ID;
		
		public uint ID;
		public uint splineId;
		public uint ordinal;
		public float position0;
		public float position1;
		public float position2;
		public float facing0;
		public float facing1;
		public float facing2;
		public float facing3;
		public uint eventId;
		public float frameTime;
		public float delay;
		public float fovy;
	}
}
