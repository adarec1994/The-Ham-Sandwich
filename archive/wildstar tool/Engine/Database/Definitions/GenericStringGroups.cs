namespace ProjectWS.Engine.Database.Definitions
{
	public class GenericStringGroups : TblRecord
	{
		public override string GetFileName() => "GenericStringGroups";
		public override uint GetID() => this.ID;
		
		public uint ID;
		public uint localizedTextId;
	}
}
