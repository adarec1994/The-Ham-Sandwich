namespace MathUtils
{
    public static class MatrixExtensions
    {

        public static Matrix4 TRS(this Matrix4 res, Vector3 t, Quaternion r, Vector3 s)
        {
            res = Matrix4.Identity;
            res *= Matrix4.CreateScale(s);
            res *= Matrix4.CreateFromQuaternion(r);
            res *= Matrix4.CreateTranslation(t);

            return res;
        }

        public static Vector3 ExtractPosition(this Matrix4 matrix)
        {
            return matrix.ExtractTranslation();
        }
    }
}