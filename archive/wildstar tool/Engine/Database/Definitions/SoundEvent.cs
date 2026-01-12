namespace ProjectWS.Engine.Database.Definitions
{
	public class SoundEvent : TblRecord
	{
		public override string GetFileName() => "SoundEvent";
		public override uint GetID() => this.ID;
		
		public uint ID;
		public string name;
		public uint hash;
		public float radius;
		public uint soundBankId00;
		public uint soundBankId01;
		public uint soundBankId02;
		public uint soundBankId03;
		public uint soundBankId04;
		public uint soundBankId05;
		public uint soundBankId06;
		public uint soundBankId07;
		public uint flags;
		public uint limitPriority;
	}
}
