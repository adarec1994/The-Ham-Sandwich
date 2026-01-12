namespace ProjectWS.Engine.Database.Definitions
{
	public class Vital : TblRecord
	{
		public override string GetFileName() => "Vital";
		public override uint GetID() => this.ID;
		
		public uint ID;
		public uint localizedTextIdDisplayText;
	}
}
