namespace ProjectWS.Engine.Database.Definitions
{
	public class InputAction : TblRecord
	{
		public override string GetFileName() => "InputAction";
		public override uint GetID() => this.ID;
		
		public uint ID;
		public string enumName;
		public uint localizedTextId;
		public uint inputActionCategoryId;
		public uint canHaveUpDownState;
		public uint displayIndex;
	}
}
