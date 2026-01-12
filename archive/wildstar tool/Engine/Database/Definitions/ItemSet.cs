namespace ProjectWS.Engine.Database.Definitions
{
	public class ItemSet : TblRecord
	{
		public override string GetFileName() => "ItemSet";
		public override uint GetID() => this.ID;
		
		public uint ID;
		public uint localizedTextId;
		public uint itemSetBonusId00;
		public uint itemSetBonusId01;
		public uint itemSetBonusId02;
		public uint itemSetBonusId03;
		public uint itemSetBonusId04;
		public uint itemSetBonusId05;
		public uint itemSetBonusId06;
		public uint itemSetBonusId07;
		public uint itemSetBonusId08;
		public uint itemSetBonusId09;
		public uint itemSetBonusId10;
		public uint itemSetBonusId11;
	}
}
