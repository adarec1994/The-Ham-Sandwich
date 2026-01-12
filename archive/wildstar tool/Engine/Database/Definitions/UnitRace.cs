namespace ProjectWS.Engine.Database.Definitions
{
	public class UnitRace : TblRecord
	{
		public override string GetFileName() => "UnitRace";
		public override uint GetID() => this.ID;
		
		public uint ID;
		public uint soundImpactDescriptionIdOrigin;
		public uint soundImpactDescriptionIdTarget;
		public uint unitVisualTypeId;
		public uint soundEventIdAggro;
		public uint soundEventIdAware;
		public uint soundSwitchIdModel;
		public uint soundCombatLoopId;
	}
}
