namespace ProjectWS.Engine.Database.Definitions
{
	public class ModelSequenceTransition : TblRecord
	{
		public override string GetFileName() => "ModelSequenceTransition";
		public override uint GetID() => this.ID;
		
		public uint ID;
		public uint modelSequenceIdFrom;
		public uint modelSequenceIdTo;
		public uint modelSequenceIdTransition;
	}
}
