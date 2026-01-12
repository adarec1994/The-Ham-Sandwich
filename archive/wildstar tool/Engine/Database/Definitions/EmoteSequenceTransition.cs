namespace ProjectWS.Engine.Database.Definitions
{
	public class EmoteSequenceTransition : TblRecord
	{
		public override string GetFileName() => "EmoteSequenceTransition";
		public override uint GetID() => this.ID;
		
		public uint ID;
		public uint emotesIdTo;
		public uint standStateFrom;
		public uint emotesIdFrom;
		public uint modelSequenceId;
	}
}
