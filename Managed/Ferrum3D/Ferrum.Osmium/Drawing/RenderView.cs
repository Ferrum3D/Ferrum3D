using Ferrum.Core.Math;

namespace Ferrum.Osmium.Drawing
{
    public sealed class RenderView
    {
        public RenderViewType Type { get; }

        public Matrix4x4F ViewMatrix
        {
            get
            {
                if (needUpdateView)
                {
                    viewMatrix = Matrix4x4F.CreateTransform(rotation, position);
                    needUpdateView = false;
                }

                return viewMatrix;
            }
        }

        public Matrix4x4F ProjectionMatrix
        {
            get
            {
                if (needUpdateProjection)
                {
                    projectionMatrix = Matrix4x4F.CreateProjection(fieldOfView, aspectRatio, nearClippingPlane, farClippingPlane);
                    needUpdateProjection = false;
                }

                return projectionMatrix;
            }
        }

        public Vector3F Position
        {
            get => position;
            set
            {
                position = value;
                needUpdateView = true;
            }
        }

        public Quaternion Rotation
        {
            get => rotation;
            set
            {
                rotation = value;
                needUpdateView = true;
            }
        }

        public float FieldOfView
        {
            get => fieldOfView;
            set
            {
                fieldOfView = value;
                needUpdateProjection = true;
            }
        }

        public float AspectRatio
        {
            get => aspectRatio;
            set
            {
                aspectRatio = value;
                needUpdateProjection = true;
            }
        }

        public float NearClippingPlane
        {
            get => nearClippingPlane;
            set
            {
                nearClippingPlane = value;
                needUpdateProjection = true;
            }
        }

        public float FarClippingPlane
        {
            get => farClippingPlane;
            set
            {
                farClippingPlane = value;
                needUpdateProjection = true;
            }
        }

        public DrawPacket DrawPacket { get; set; }
        public DrawItemSortSettings SortSettings { get; set; }

        private Matrix4x4F viewMatrix;
        private Matrix4x4F projectionMatrix;
        private bool needUpdateView = true;
        private bool needUpdateProjection = true;

        private Vector3F position;
        private Quaternion rotation;

        private float fieldOfView = MathF.PI * 0.5f;
        private float aspectRatio = 1.0f;
        private float nearClippingPlane = 0.1f;
        private float farClippingPlane = 1000.0f;

        private RenderView(RenderViewType type)
        {
            Type = type;
        }

        public static RenderView CreateCamera(float aspectRatio)
        {
            return new RenderView(RenderViewType.Camera)
            {
                aspectRatio = aspectRatio,
                SortSettings = new DrawItemSortSettings(DrawItemCompareOp.Less)
            };
        }

        public void Translate(float x, float y, float z)
        {
            Position += new Vector3F(x, y, z);
        }

        public void TranslateX(float x)
        {
            Position += Vector3F.UnitX * x;
        }

        public void TranslateY(float y)
        {
            Position += Vector3F.UnitY * y;
        }

        public void TranslateZ(float z)
        {
            Position += Vector3F.UnitZ * z;
        }

        public void Rotate(Vector3F axis, float angle)
        {
            Rotation *= Quaternion.FromAxisAngle(axis, angle);
        }

        public void RotateX(float x)
        {
            Rotation *= Quaternion.CreateRotationX(x);
        }

        public void RotateY(float y)
        {
            Rotation *= Quaternion.CreateRotationY(y);
        }

        public void RotateZ(float z)
        {
            Rotation *= Quaternion.CreateRotationZ(z);
        }
    }
}
