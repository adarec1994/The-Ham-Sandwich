namespace ProjectWS.Engine.Database.Definitions
{
	public class HousingBuild : TblRecord
	{
		public override string GetFileName() => "HousingBuild";
		public override uint GetID() => this.ID;
		
		public uint ID;
		public string description;
		public string assetPath;
		public uint constructionEffectsId;
		public float buildPreDelayTimeMS;
		public float buildPostDelayTimeMS;
		public float buildTime00;
		public float buildTime01;
		public float buildTime02;
		public float buildTime03;
		public float buildTime04;
		public float buildTime05;
		public float buildTime06;
		public float buildTime07;
		public uint modelSequenceId00;
		public uint modelSequenceId01;
		public uint modelSequenceId02;
		public uint modelSequenceId03;
		public uint modelSequenceId04;
		public uint modelSequenceId05;
		public uint modelSequenceId06;
		public uint modelSequenceId07;
	}
}
