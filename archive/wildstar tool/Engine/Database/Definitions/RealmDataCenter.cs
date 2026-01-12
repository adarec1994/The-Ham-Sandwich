namespace ProjectWS.Engine.Database.Definitions
{
	public class RealmDataCenter : TblRecord
	{
		public override string GetFileName() => "RealmDataCenter";
		public override uint GetID() => this.ID;
		
		public uint ID;
		public uint flags;
		public uint deploymentRegionEnum;
		public uint deploymentTypeEnum;
		public uint localizedTextId;
		public string authServer;
		public string ncClientAuthServer;
		public string ncRedirectUrlTemplate;
		public string ncRedirectUrlTemplateSignature;
		public string ncAppGroupCode;
		public uint ncProgramAuth;
		public string steamSignatureUrlTemplate;
		public string steamNCoinUrlTemplate;
		public string storeBannerDataUrlTemplate;
	}
}
