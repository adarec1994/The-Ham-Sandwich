namespace ProjectWS.Engine.Database.Definitions
{
	public class LuaEvent : TblRecord
	{
		public override string GetFileName() => "LuaEvent";
		public override uint GetID() => this.ID;
		
		public uint ID;
		public string eventName;
		public string parameters;
	}
}
