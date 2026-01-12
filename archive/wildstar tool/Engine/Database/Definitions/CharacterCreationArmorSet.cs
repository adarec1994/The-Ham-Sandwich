namespace ProjectWS.Engine.Database.Definitions
{
	public class CharacterCreationArmorSet : TblRecord
	{
		public override string GetFileName() => "CharacterCreationArmorSet";
		public override uint GetID() => this.ID;
		
		public uint ID;
		public uint creationGearSetEnum;
		public uint classId;
		public uint itemDisplayId00;
		public uint itemDisplayId01;
		public uint itemDisplayId02;
		public uint itemDisplayId03;
		public uint itemDisplayId04;
		public uint itemDisplayId05;
		public uint itemDisplayId06;
		public uint itemDisplayId07;
		public uint itemDisplayId08;
		public uint itemDisplayId09;
		public uint itemDisplayId10;
		public uint itemDisplayId11;
	}
}
