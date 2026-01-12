namespace ProjectWS.Engine.Database.Definitions
{
	public class BindPoint : TblRecord
	{
		public override string GetFileName() => "BindPoint";
		public override uint GetID() => this.ID;
		
		public uint ID;
		public uint bindPointFactionEnum;
		public uint localizedTextId;
	}
}
