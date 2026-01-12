namespace ProjectWS.Engine.Database.Definitions
{
	public class ActionSlotPrereq : TblRecord
	{
		public override string GetFileName() => "ActionSlotPrereq";
		public override uint GetID() => this.ID;
		
		public uint ID;
		public uint slotIndex;
		public uint prerequisiteIdUnlock;
	}
}
