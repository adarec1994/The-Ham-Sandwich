namespace ProjectWS.Engine.Database.Definitions
{
	public class DatacubeVolume : TblRecord
	{
		public override string GetFileName() => "DatacubeVolume";
		public override uint GetID() => this.ID;
		
		public uint ID;
		public uint localizedTextIdName;
		public string assetPath;
		public uint datacubeId00;
		public uint datacubeId01;
		public uint datacubeId02;
		public uint datacubeId03;
		public uint datacubeId04;
		public uint datacubeId05;
		public uint datacubeId06;
		public uint datacubeId07;
		public uint datacubeId08;
		public uint datacubeId09;
		public uint datacubeId10;
		public uint datacubeId11;
		public uint datacubeId12;
		public uint datacubeId13;
		public uint datacubeId14;
		public uint datacubeId15;
	}
}
