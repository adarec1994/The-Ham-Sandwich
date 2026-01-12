namespace ProjectWS.FileFormats.Sky
{
    public class SHCoefficients
    {
        public float[] coefficients;

        public SHCoefficients(BinaryReader br)
        {
            this.coefficients = new float[27];

            for (int i = 0; i < 27; i++)
            {
                this.coefficients[i] = br.ReadSingle();
            }
        }
    }

}
