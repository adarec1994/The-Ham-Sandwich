namespace ProjectWS.Engine.Database.Definitions
{
	public class WVisualEffect : TblRecord
	{
		public override string GetFileName() => "VisualEffect";
		public override uint GetID() => this.ID;
		
		public uint ID;
		public uint visualType;
		public uint startDelay;
		public uint duration;
		public uint modelItemSlot;
		public uint modelItemCostumeSide;
		public string modelAssetPath;
		public uint modelAttachmentId;
		public uint modelSequencePriority;
		public uint modelSequenceIdTarget00;
		public uint modelSequenceIdTarget01;
		public uint modelSequenceIdTarget02;
		public float modelScale;
		public float modelRotationX;
		public float modelRotationY;
		public float modelRotationZ;
		public float data00;
		public float data01;
		public float data02;
		public float data03;
		public float data04;
		public uint flags;
		public uint soundEventId00;
		public uint soundEventId01;
		public uint soundEventId02;
		public uint soundEventId03;
		public uint soundEventId04;
		public uint soundEventId05;
		public uint soundEventOffset00;
		public uint soundEventOffset01;
		public uint soundEventOffset02;
		public uint soundEventOffset03;
		public uint soundEventOffset04;
		public uint soundEventOffset05;
		public uint soundEventIdStop;
		public uint soundZoneKitId;
		public uint prerequisiteId;
		public uint particleDiffuseColor;
	}
}
