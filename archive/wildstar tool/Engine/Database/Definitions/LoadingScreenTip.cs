namespace ProjectWS.Engine.Database.Definitions
{
	public class LoadingScreenTip : TblRecord
	{
		public override string GetFileName() => "LoadingScreenTip";
		public override uint GetID() => this.ID;
		
		public uint ID;
		public uint localizedTextId;
	}
}
