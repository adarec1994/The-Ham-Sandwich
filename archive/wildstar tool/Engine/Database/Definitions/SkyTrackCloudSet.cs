namespace ProjectWS.Engine.Database.Definitions
{
	public class SkyTrackCloudSet : TblRecord
	{
		public override string GetFileName() => "SkyTrackCloudSet";
		public override uint GetID() => this.ID;
		
		public uint ID;
		public uint count;
		public float minSize00;
		public float minSize01;
		public float minSize02;
		public float minSize03;
		public float minSize04;
		public float minSize05;
		public float minSize06;
		public float minSize07;
		public float minSize08;
		public float minSize09;
		public float minSize10;
		public float minSize11;
		public float maxSize00;
		public float maxSize01;
		public float maxSize02;
		public float maxSize03;
		public float maxSize04;
		public float maxSize05;
		public float maxSize06;
		public float maxSize07;
		public float maxSize08;
		public float maxSize09;
		public float maxSize10;
		public float maxSize11;
		public string model00;
		public string model01;
		public string model02;
		public string model03;
		public string model04;
		public string model05;
		public string model06;
		public string model07;
		public string model08;
		public string model09;
		public string model10;
		public string model11;
	}
}
