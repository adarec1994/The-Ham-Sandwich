namespace ProjectWS.Engine.Database.Definitions
{
	public class Quest2RandomTextLineJoin : TblRecord
	{
		public override string GetFileName() => "Quest2RandomTextLineJoin";
		public override uint GetID() => this.ID;
		
		public uint ID;
		public uint quest2Id;
		public uint questVOTextType;
		public uint randomTextLineId;
	}
}
