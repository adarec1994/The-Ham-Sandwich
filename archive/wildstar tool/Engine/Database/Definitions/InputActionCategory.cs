namespace ProjectWS.Engine.Database.Definitions
{
	public class InputActionCategory : TblRecord
	{
		public override string GetFileName() => "InputActionCategory";
		public override uint GetID() => this.ID;
		
		public uint ID;
		public uint localizedTextId;
	}
}
