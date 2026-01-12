namespace ProjectWS.Engine.Database.Definitions
{
	public class GenericMapNode : TblRecord
	{
		public override string GetFileName() => "GenericMapNode";
		public override uint GetID() => this.ID;
		
		public uint ID;
		public uint genericMapId;
		public uint worldLocation2Id;
		public uint localizedTextIdName;
		public uint localizedTextIdDescription;
		public string spritePath;
		public uint genericMapNodeTypeEnum;
		public uint flags;
	}
}
