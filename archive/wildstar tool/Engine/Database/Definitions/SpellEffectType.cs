namespace ProjectWS.Engine.Database.Definitions
{
	public class SpellEffectType : TblRecord
	{
		public override string GetFileName() => "SpellEffectType";
		public override uint GetID() => this.ID;
		
		public uint ID;
		public uint flags;
		public uint dataType00;
		public uint dataType01;
		public uint dataType02;
		public uint dataType03;
		public uint dataType04;
		public uint dataType05;
		public uint dataType06;
		public uint dataType07;
		public uint dataType08;
		public uint dataType09;
	}
}
