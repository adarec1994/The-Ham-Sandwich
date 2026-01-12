namespace ProjectWS.Engine.Database.Definitions
{
	public class Prerequisite : TblRecord
	{
		public override string GetFileName() => "Prerequisite";
		public override uint GetID() => this.ID;
		
		public uint ID;
		public uint flags;
		public uint prerequisiteTypeId0;
		public uint prerequisiteTypeId1;
		public uint prerequisiteTypeId2;
		public uint prerequisiteComparisonId0;
		public uint prerequisiteComparisonId1;
		public uint prerequisiteComparisonId2;
		public uint objectId0;
		public uint objectId1;
		public uint objectId2;
		public uint value0;
		public uint value1;
		public uint value2;
		public uint localizedTextIdFailure;
	}
}
