namespace ProjectWS.Engine.Database.Definitions
{
	public class Creature2OutfitInfo : TblRecord
	{
		public override string GetFileName() => "Creature2OutfitInfo";
		public override uint GetID() => this.ID;
		
		public uint ID;
		public uint itemDisplayId00;
		public uint itemDisplayId01;
		public uint itemDisplayId02;
		public uint itemDisplayId03;
		public uint itemDisplayId04;
		public uint itemDisplayId05;
		public uint itemColorSetId00;
		public uint itemColorSetId01;
		public uint itemColorSetId02;
		public uint itemColorSetId03;
		public uint itemColorSetId04;
		public uint itemColorSetId05;
	}
}
