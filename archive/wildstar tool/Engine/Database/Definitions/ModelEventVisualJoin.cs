namespace ProjectWS.Engine.Database.Definitions
{
	public class ModelEventVisualJoin : TblRecord
	{
		public override string GetFileName() => "ModelEventVisualJoin";
		public override uint GetID() => this.ID;
		
		public uint ID;
		public uint unitVisualTypeId;
		public uint itemVisualTypeId;
		public uint materialTypeId;
		public uint modelEventId;
		public uint visualEffectId;
		public uint modelSequenceId;
		public uint flags;
	}
}
