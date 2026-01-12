namespace ProjectWS.Engine.Database.Definitions
{
	public class PathSettlerSheriff : TblRecord
	{
		public override string GetFileName() => "PathSettlerSheriff";
		public override uint GetID() => this.ID;
		
		public uint ID;
		public uint quest2IdSheriff00;
		public uint quest2IdSheriff01;
		public uint quest2IdSheriff02;
		public uint quest2IdSheriff03;
		public uint quest2IdSheriff04;
		public uint quest2IdSheriff05;
		public uint quest2IdSheriff06;
		public uint quest2IdSheriff07;
		public uint localizedTextIdDescription00;
		public uint localizedTextIdDescription01;
		public uint localizedTextIdDescription02;
		public uint localizedTextIdDescription03;
		public uint localizedTextIdDescription04;
		public uint localizedTextIdDescription05;
		public uint localizedTextIdDescription06;
		public uint localizedTextIdDescription07;
		public uint characterTitleIdReward;
	}
}
