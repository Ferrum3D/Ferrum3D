using Ferrum.Core.Math;
using Ferrum.Osmium.GPU.DeviceObjects;
using Ferrum.Osmium.GPU.WindowSystem;

namespace Ferrum.Osmium.Drawing
{
    public sealed class RenderView
    {
        public RenderViewType Type { get; }

        public SwapChain SwapChain { get; }

        public Matrix4x4F ViewMatrix
        {
            get
            {
                if (needUpdateView)
                {
                    viewMatrix = Matrix4x4F.CreateTranslation(position);
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

        private float fieldOfView = MathF.PI * 0.5f;
        private float aspectRatio = 1.0f;
        private float nearClippingPlane = 0.1f;
        private float farClippingPlane = 1000.0f;

        private RenderView(RenderViewType type, SwapChain swapChain = null)
        {
            Type = type;
            SwapChain = swapChain;
        }

        public static RenderView CreateMainCamera(Window window, SwapChain swapChain)
        {
            return new RenderView(RenderViewType.Camera, swapChain)
            {
                aspectRatio = swapChain.AspectRatio,
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
    }
}
