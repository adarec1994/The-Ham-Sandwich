using MathUtils;
using OpenTK.Audio.OpenAL;
using OpenTK.Graphics.ES20;
using OpenTK.Windowing.GraphicsLibraryFramework;

namespace ProjectWS.Engine.Components
{
    public class CameraController : Component
    {
        // Defines several possible options for camera movement. Used as abstraction to stay away from window-system specific input methods
        public enum CameraMovement
        {
            FORWARD,
            BACKWARD,
            LEFT,
            RIGHT,
            UP,
            DOWN
        }

        // Default camera values
        const float YAW = 120.0f;
        const float PITCH = 0.0f;
        const float SPEED = 50.0f;
        const float SENSITIVITY_FLY = 0.1f;
        const float SENSITIVITY_ORBIT = 0.2f;
        const float SENSITIVITY_ORTHO = 0.1f;
        const float ZOOM = 45.0f;
        const float DISTANCE = 5.0f;

        // camera Attributes
        public Vector3 Pos;
        public Vector3 Front;
        public Vector3 Up;
        public Vector3 Right;
        public Vector3 WorldUp;
        public float distanceToOrigin;
        public Vector3 lookAtPoint = Vector3.UnitY; // placing this at (0, 1, 0)
                                                    // so the camera looks more at the middle of the loaded model
                                                    // mainly until I add model framing based on bounding box

        // euler Angles
        public float Yaw;
        public float Pitch;

        // camera options
        public float MovementSpeed;
        public float MouseSensitivity_Fly;
        public float MouseSensitivity_Orbit;
        public float MouseSensitivity_Ortho;
        public float Zoom;

        public enum Mode
        {
            Fly,
            Orbit,
            OrthoTop,
        }

        public Mode cameraMode = Mode.Orbit;

        Camera camera;
        Input.Input input;
        int rendererID;

        public CameraController(Camera camera, Input.Input input) : base()
        {
            SetDefaults();

            this.camera = camera;
            this.input = input;
            this.rendererID = this.camera.renderer.ID;

            this.WorldUp = Vector3.UnitY;
            UpdateCameraVectors();
        }

        void SetDefaults()
        {
            this.Front = new Vector3(0.0f, 0.0f, 1.0f);
            this.Up = Vector3.UnitY;
            this.Yaw = YAW;
            this.Pitch = PITCH;
            this.MovementSpeed = SPEED;
            this.MouseSensitivity_Fly = SENSITIVITY_FLY;
            this.MouseSensitivity_Orbit = SENSITIVITY_ORBIT;
            this.MouseSensitivity_Ortho = SENSITIVITY_ORTHO;
            this.Zoom = ZOOM;
            this.distanceToOrigin = DISTANCE;
        }

        public void Teleport(float x, float y, float z)
        {
            this.Pos = new Vector3(x, y, z);
        }

        // processes input received from any keyboard-like input system. Accepts input parameter in the form of camera defined ENUM (to abstract it from windowing systems)
        public void ProcessKeyboard(CameraMovement direction, float deltaTime)
        {
            if (cameraMode == Mode.Fly)
            {
                float speed = this.MovementSpeed * deltaTime;
                if (direction == CameraMovement.FORWARD)
                    this.Pos += (speed * this.Front);
                if (direction == CameraMovement.BACKWARD)
                    this.Pos -= (speed * this.Front);
                if (direction == CameraMovement.LEFT)
                    this.Pos -= Vector3.Normalize(Vector3.Cross(this.Front, this.Up)) * speed;
                if (direction == CameraMovement.RIGHT)
                    this.Pos += Vector3.Normalize(Vector3.Cross(this.Front, this.Up)) * speed;
                if (direction == CameraMovement.UP)
                    this.Pos += (speed * this.Up);
                if (direction == CameraMovement.DOWN)
                    this.Pos -= (speed * this.Up);
            }
        }

        // processes input received from a mouse input system. Expects the offset value in both the x and y direction.
        public void ProcessMouseMovement(float xoffset, float yoffset, bool constrainPitch = true)
        {
            if (this.cameraMode == Mode.Fly)
            {
                xoffset *= this.MouseSensitivity_Fly;
                yoffset *= this.MouseSensitivity_Fly;
            }
            else if (this.cameraMode == Mode.Orbit)
            {
                xoffset *= this.MouseSensitivity_Orbit;
                yoffset *= this.MouseSensitivity_Orbit;
            }
            else if (this.cameraMode == Mode.OrthoTop)
            {
                var sensitivity = (this.MouseSensitivity_Ortho / (this.camera as OrthoCamera).zoom * 10);
                xoffset *= sensitivity;
                yoffset *= sensitivity;
            }

            this.Yaw += xoffset;
            if (this.cameraMode == Mode.Fly)
                this.Pitch += yoffset;
            else if (this.cameraMode == Mode.Orbit)
                this.Pitch -= yoffset;
            else if (this.cameraMode == Mode.OrthoTop)
            {
                if (this.input.mouseDownInView)
                {
                    this.Pos.X -= xoffset;
                    this.Pos.Z += yoffset;
                }
            }

            // make sure that when pitch is out of bounds, screen doesn't get flipped
            if (constrainPitch)
            {
                if (this.Pitch > 89.0f)
                    this.Pitch = 89.0f;
                if (this.Pitch < -89.0f)
                    this.Pitch = -89.0f;
            }

            // update Front, Right and Up Vectors using the updated Euler angles
            UpdateCameraVectors();
        }

        public void ProcessMousePan(float xoffset, float yoffset)
        {
            if (this.cameraMode == Mode.Orbit)
            {
                this.lookAtPoint.Y -= yoffset * 0.01f;
            }
        }

        // processes input received from a mouse scroll-wheel event. Only requires input on the vertical wheel-axis
        public void ProcessMouseScroll(float scroll)
        {
            if (this.cameraMode == Mode.Fly)
            {
                this.Zoom -= scroll;
                if (this.Zoom < 1.0f)
                    this.Zoom = 1.0f;
                if (this.Zoom > 45.0f)
                    this.Zoom = 45.0f;
            }
            else if (this.cameraMode == Mode.Orbit)
            {
                this.distanceToOrigin -= scroll * 0.5f;
                if (this.distanceToOrigin <= 0.001f)
                    this.distanceToOrigin = 0.001f;

                UpdateCameraVectors();
            }
            else if (this.cameraMode == Mode.OrthoTop)
            {
                var oCamera = this.camera as OrthoCamera;
                if (oCamera != null)
                {
                    oCamera.zoom += scroll * (oCamera.zoom / 10f);

                    if (oCamera.zoom <= 1)
                        oCamera.zoom = 1;

                    if (oCamera.zoom >= 1000)
                        oCamera.zoom = 1000;
                }
                //this.Pos.Y -= scroll;
            }
        }

        // calculates the front vector from the Camera's (updated) Euler Angles
        void UpdateCameraVectors()
        {
            // calculate the new Front vector
            Vector3 front;
            front.X = (float)(Math.Cos(MathHelper.DegreesToRadians(this.Yaw)) * Math.Cos(MathHelper.DegreesToRadians(this.Pitch)));
            front.Y = (float)Math.Sin(MathHelper.DegreesToRadians(this.Pitch));
            front.Z = (float)(Math.Sin(MathHelper.DegreesToRadians(this.Yaw)) * Math.Cos(MathHelper.DegreesToRadians(this.Pitch)));

            if (cameraMode == Mode.Orbit)
            {
                this.Front = Vector3.Normalize(front);
                this.Pos = front * this.distanceToOrigin;
            }
            else if (cameraMode == Mode.Fly)
            {
                this.Front = Vector3.Normalize(front);
            }
            else if (cameraMode == Mode.OrthoTop)
            {
                this.Front = Vector3.Normalize(front);
            }

            // also re-calculate the Right and Up vector
            this.Right = Vector3.Normalize(Vector3.Cross(this.Front, this.WorldUp));  // normalize the vectors, because their length gets closer to 0 the more you look up or down which results in slower movement.
            //this.Up = Vector3.Normalize(Vector3.Cross(this.Right, this.Front));
        }

        public override void Update(float deltaTime)
        {
            if (this.camera.renderer == null) return;
            if (this.camera.renderer.engine == null) return;

            if (this.rendererID == this.camera.renderer.engine.focusedRendererID)
            {
                if (this.input.GetKeyDown(Keys.W))
                    ProcessKeyboard(CameraMovement.FORWARD, deltaTime);
                if (this.input.GetKeyDown(Keys.S))
                    ProcessKeyboard(CameraMovement.BACKWARD, deltaTime);
                if (this.input.GetKeyDown(Keys.A))
                    ProcessKeyboard(CameraMovement.LEFT, deltaTime);
                if (this.input.GetKeyDown(Keys.D))
                    ProcessKeyboard(CameraMovement.RIGHT, deltaTime);
                if (this.input.GetKeyDown(Keys.Space))
                    ProcessKeyboard(CameraMovement.UP, deltaTime);
                if (this.input.GetKeyDown(Keys.C))
                    ProcessKeyboard(CameraMovement.DOWN, deltaTime);

                if (this.camera != null)
                {
                    var mouseDiff = this.input.GetMouseDiff();
                    if (this.input.RMB)   // RMB
                    {
                        ProcessMouseMovement(mouseDiff.X, mouseDiff.Y);
                    }
                    if (this.input.MMB)   // MMB
                    {
                        ProcessMousePan(mouseDiff.X, mouseDiff.Y);
                    }
                    ProcessMouseScroll(this.input.GetMouseScroll());
                }
            }

            Matrix4 cameraMat;

            if (this.cameraMode == Mode.Orbit)
            {
                cameraMat = Matrix4.LookAt(this.Pos + this.lookAtPoint, this.lookAtPoint, this.Up);
            }
            else if (this.cameraMode == Mode.Fly)
            {
                cameraMat = Matrix4.LookAt(this.Pos, this.Pos + this.Front, this.Up);
            }
            else if (this.cameraMode == Mode.OrthoTop)
            {
                cameraMat = Matrix4.Identity;
                //cameraMat *= Matrix4.CreateRotationZ((MathF.PI / 180) * 90);
                cameraMat = Matrix4.LookAt(this.Pos, this.Pos - (this.Up * 100), Vector3.UnitZ);
                //cameraMat *= Matrix4.CreateTranslation(this.Pos);
            }
            else
            {
                cameraMat = Matrix4.Identity;
            }

            if (this.camera != null)
            {
                this.camera.transform.SetRotation(cameraMat.ExtractRotation());
                this.camera.transform.SetPosition(this.Pos);
                //this.camera.transform.SetMatrix(cameraMat);
                this.camera.view = cameraMat;
            }
        }
    }
}
