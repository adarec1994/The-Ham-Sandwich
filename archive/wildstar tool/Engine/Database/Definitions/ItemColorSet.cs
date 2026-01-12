namespace ProjectWS.Engine.Database.Definitions
{
	public class ItemColorSet : TblRecord
	{
		public override string GetFileName() => "ItemColorSet";
		public override uint GetID() => this.ID;
		
		public uint ID;
		public uint dyeColorRampId00;
		public uint dyeColorRampId01;
		public uint dyeColorRampId02;
	}
}
