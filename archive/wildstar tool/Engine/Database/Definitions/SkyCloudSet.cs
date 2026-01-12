namespace ProjectWS.Engine.Database.Definitions
{
	public class SkyCloudSet : TblRecord
	{
		public override string GetFileName() => "SkyCloudSet";
		public override uint GetID() => this.ID;
		
		public uint ID;
		public float density;
		public uint skyTrackCloudSetId00;
		public uint skyTrackCloudSetId01;
		public uint skyTrackCloudSetId02;
		public uint skyTrackCloudSetId03;
		public uint skyTrackCloudSetId04;
		public uint skyTrackCloudSetId05;
	}
}
