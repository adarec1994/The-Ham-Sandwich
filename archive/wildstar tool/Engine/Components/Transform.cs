using MathUtils;

namespace ProjectWS.Engine.Components
{
    public class Transform : Component
    {
        Vector3 position;
        Quaternion rotation;
        Vector3 scale;

        bool needsUpdate;
        Matrix4 matrix;

        public void SetPosition(Vector3 position)
        {
            this.position = position;
            this.needsUpdate = true;
        }

        public void SetPosition(float x, float y, float z)
        {
            this.position.X = x;
            this.position.Y = y;
            this.position.Z = z;
            this.needsUpdate = true;
        }

        public void SetRotation(Quaternion rotation)
        {
            this.rotation = rotation;
            this.needsUpdate = true;
        }

        public void SetScale(Vector3 scale)
        {
            this.scale = scale;
            this.needsUpdate = true;
        }

        public Vector3 GetPosition()
        {
            return this.position;
        }

        public Quaternion GetRotation()
        {
            return this.rotation;
        }

        public Vector3 GetScale()
        {
            return this.scale;
        }

        public Transform()
        {
            this.position = Vector3.Zero;
            this.rotation = Quaternion.Identity;
            this.scale = Vector3.One;
            this.needsUpdate = true;
        }

        public Transform(Vector3 position, Quaternion rotation, Vector3 scale)
        {
            this.position = position;
            this.rotation = rotation;
            this.scale = scale;
            this.needsUpdate = true;
        }

        public Matrix4 GetMatrix()
        {
            if (this.needsUpdate)
            {
                this.matrix = Matrix4.CreateScale(scale) * Matrix4.CreateFromQuaternion(rotation) * Matrix4.CreateTranslation(position);

                //this.matrix = Matrix4.Identity;
                //this.matrix *= Matrix4.CreateScale(scale);
                //this.matrix *= Matrix4.CreateFromQuaternion(rotation);
                //this.matrix *= Matrix4.CreateTranslation(position);
                this.needsUpdate = false;
            }
            return this.matrix;
        }

        public void SetMatrix(Matrix4 mat)
        {
            // This doesn't work for some reason
            this.needsUpdate = false;
            this.matrix = new Matrix4(mat.Row0, mat.Row1, mat.Row2, mat.Row3);//mat;
        }

        public override void Update(float deltaTime) { }
    }
}
