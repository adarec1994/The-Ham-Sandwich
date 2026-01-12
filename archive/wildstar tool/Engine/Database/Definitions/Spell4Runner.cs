namespace ProjectWS.Engine.Database.Definitions
{
	public class Spell4Runner : TblRecord
	{
		public override string GetFileName() => "Spell4Runner";
		public override uint GetID() => this.ID;
		
		public uint ID;
		public string spriteName;
		public float redTint;
		public float greenTint;
		public float blueTint;
		public float alphaTint;
		public float rate;
	}
}
