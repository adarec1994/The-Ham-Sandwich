namespace ProjectWS.Engine.Database.Definitions
{
	public class ItemRuneInstance : TblRecord
	{
		public override string GetFileName() => "ItemRuneInstance";
		public override uint GetID() => this.ID;
		
		public uint ID;
		public uint definedSocketCount;
		public uint definedSocketType00;
		public uint definedSocketType01;
		public uint definedSocketType02;
		public uint definedSocketType03;
		public uint definedSocketType04;
		public uint definedSocketType05;
		public uint definedSocketType06;
		public uint definedSocketType07;
		public uint itemSetId;
		public uint itemSetPower;
		public uint socketCountMax;
	}
}
