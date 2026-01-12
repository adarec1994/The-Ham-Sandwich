namespace ProjectWS.Engine.Database.Definitions
{
	public class PetFlair : TblRecord
	{
		public override string GetFileName() => "PetFlair";
		public override uint GetID() => this.ID;
		
		public uint ID;
		public uint unlockBitIndex00;
		public uint unlockBitIndex01;
		public uint type;
		public uint spell4Id;
		public uint localizedTextIdTooltip;
		public uint itemDisplayId00;
		public uint itemDisplayId01;
		public uint prerequisiteId;
	}
}
