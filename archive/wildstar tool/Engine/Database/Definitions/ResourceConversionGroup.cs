namespace ProjectWS.Engine.Database.Definitions
{
	public class ResourceConversionGroup : TblRecord
	{
		public override string GetFileName() => "ResourceConversionGroup";
		public override uint GetID() => this.ID;
		
		public uint ID;
		public uint flags;
		public uint resourceConversionId00;
		public uint resourceConversionId01;
		public uint resourceConversionId02;
		public uint resourceConversionId03;
		public uint resourceConversionId04;
		public uint resourceConversionId05;
	}
}
