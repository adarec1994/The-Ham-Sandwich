namespace ProjectWS.Engine.Database.Definitions
{
	public class TargetGroup : TblRecord
	{
		public override string GetFileName() => "TargetGroup";
		public override uint GetID() => this.ID;
		
		public uint ID;
		public uint localizedTextIdDisplayString;
		public uint type;
		public uint data0;
		public uint data1;
		public uint data2;
		public uint data3;
		public uint data4;
		public uint data5;
		public uint data6;
	}
}
