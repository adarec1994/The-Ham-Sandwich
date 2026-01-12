namespace ProjectWS.Engine.Database.Definitions
{
	public class RandomTextLine : TblRecord
	{
		public override string GetFileName() => "RandomTextLine";
		public override uint GetID() => this.ID;
		
		public uint ID;
		public uint localizedTextId;
		public uint randomTextLineSetId;
	}
}
