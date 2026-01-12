namespace ProjectWS.Engine.Database.Definitions
{
	public class HookType : TblRecord
	{
		public override string GetFileName() => "HookType";
		public override uint GetID() => this.ID;
		
		public uint ID;
	}
}
