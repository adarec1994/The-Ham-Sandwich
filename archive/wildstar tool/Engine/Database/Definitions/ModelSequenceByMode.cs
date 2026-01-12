namespace ProjectWS.Engine.Database.Definitions
{
	public class ModelSequenceByMode : TblRecord
	{
		public override string GetFileName() => "ModelSequenceByMode";
		public override uint GetID() => this.ID;
		
		public uint ID;
		public uint modelSequenceId;
		public uint modelSequenceIdSwim;
		public uint modelSequenceIdHover;
		public uint modelSequenceIdFly;
	}
}
