namespace ProjectWS.Engine.Database.Definitions
{
	public class Cinematic : TblRecord
	{
		public override string GetFileName() => "Cinematic";
		public override uint GetID() => this.ID;
		
		public uint ID;
	}
}
