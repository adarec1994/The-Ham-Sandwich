namespace ProjectWS.Engine.Database.Definitions
{
	public class UnitVehicle : TblRecord
	{
		public override string GetFileName() => "UnitVehicle";
		public override uint GetID() => this.ID;
		
		public uint ID;
		public uint vehicleTypeEnum;
		public uint numberPilots;
		public uint pilotPosture00;
		public uint pilotPosture01;
		public uint numberPassengers;
		public uint passengerPosture00;
		public uint passengerPosture01;
		public uint passengerPosture02;
		public uint passengerPosture03;
		public uint passengerPosture04;
		public uint passengerPosture05;
		public uint numberGunners;
		public uint gunnerPosture00;
		public uint gunnerPosture01;
		public uint gunnerPosture02;
		public uint gunnerPosture03;
		public uint gunnerPosture04;
		public uint gunnerPosture05;
		public uint vendorItemPrice;
		public uint localizedTextIdName;
		public uint localizedTextIdTooltip;
		public string buttonIcon;
		public uint flags;
		public uint soundEventIdTakeoff;
		public uint soundEventIdLanding;
		public uint waterSurfaceEffectIdMoving;
		public uint waterSurfaceEffectIdStanding;
		public uint waterSurfaceEffectIdJumpIn;
		public uint waterSurfaceEffectIdJumpOut;
	}
}
