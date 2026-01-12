namespace ProjectWS.Engine.Database.Definitions
{
	public class GameFormula : TblRecord
	{
		public override string GetFileName() => "GameFormula";
		public override uint GetID() => this.ID;
		
		public uint ID;
		public uint dataint0;
		public uint dataint01;
		public uint dataint02;
		public uint dataint03;
		public uint dataint04;
		public float datafloat0;
		public float datafloat01;
		public float datafloat02;
		public float datafloat03;
		public float datafloat04;
	}
}
