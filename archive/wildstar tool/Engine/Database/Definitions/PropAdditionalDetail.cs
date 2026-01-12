namespace ProjectWS.Engine.Database.Definitions
{
	public class PropAdditionalDetail : TblRecord
	{
		public override string GetFileName() => "PropAdditionalDetail";
		public override uint GetID() => this.ID;
		
		public uint ID;
		public uint localizedTextId;
	}
}
