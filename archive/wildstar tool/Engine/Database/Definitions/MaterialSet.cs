namespace ProjectWS.Engine.Database.Definitions
{
	public class MaterialSet : TblRecord
	{
		public override string GetFileName() => "MaterialSet";
		public override uint GetID() => this.ID;
		
		public uint ID;
	}
}
