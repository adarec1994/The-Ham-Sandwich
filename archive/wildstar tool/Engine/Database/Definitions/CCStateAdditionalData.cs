namespace ProjectWS.Engine.Database.Definitions
{
	public class CCStateAdditionalData : TblRecord
	{
		public override string GetFileName() => "CCStateAdditionalData";
		public override uint GetID() => this.ID;
		
		public uint ID;
		public uint ccStatesId;
		public uint dataInt00;
		public uint dataInt01;
		public uint dataInt02;
		public uint dataInt03;
		public uint dataInt04;
		public float dataFloat00;
		public float dataFloat01;
		public float dataFloat02;
		public float dataFloat03;
		public float dataFloat04;
	}
}
