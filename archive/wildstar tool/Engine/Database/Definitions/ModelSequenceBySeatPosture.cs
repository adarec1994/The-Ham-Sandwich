namespace ProjectWS.Engine.Database.Definitions
{
	public class ModelSequenceBySeatPosture : TblRecord
	{
		public override string GetFileName() => "ModelSequenceBySeatPosture";
		public override uint GetID() => this.ID;
		
		public uint ID;
		public uint modelSequenceId;
		public uint modelSequenceIdNarrow;
		public uint modelSequenceIdMedium;
		public uint modelSequenceIdWide;
		public uint modelSequenceIdBike;
		public uint modelSequenceIdHoverBoard;
		public uint modelSequenceIdGlider;
		public uint modelSequenceIdTank;
	}
}
