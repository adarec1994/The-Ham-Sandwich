namespace ProjectWS.Engine.Database.Definitions
{
	public class Creature2Action : TblRecord
	{
		public override string GetFileName() => "Creature2Action";
		public override uint GetID() => this.ID;
		
		public uint ID;
		public string description;
		public uint creatureActionSetId;
		public uint state;
		public uint wEvent;
		public uint orderIndex;
		public uint delayMS;
		public uint action;
		public uint actionData00;
		public uint actionData01;
		public uint visualEffectId;
		public uint prerequisiteId;
	}
}
