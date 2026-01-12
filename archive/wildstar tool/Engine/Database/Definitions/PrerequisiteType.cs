namespace ProjectWS.Engine.Database.Definitions
{
	public class PrerequisiteType : TblRecord
	{
		public override string GetFileName() => "PrerequisiteType";
		public override uint GetID() => this.ID;
		
		public uint ID;
		public uint localizedTextIdError;
	}
}
