using System;
using System.Linq;
using System.Runtime.CompilerServices;
using System.Runtime.InteropServices;

namespace Ferrum.Core.Math
{
    [StructLayout(LayoutKind.Sequential)]
    public partial struct Matrix4x4F : IEquatable<Matrix4x4F>
    {
        public static Matrix4x4F Zero => new();

        public static Matrix4x4F Identity => new()
        {
            row0 = Vector4F.UnitX,
            row1 = Vector4F.UnitY,
            row2 = Vector4F.UnitZ,
            row3 = Vector4F.UnitW
        };

        public Matrix4x4F Transposed => FromRows(Column0, Column1, Column2, Column3);

        public Vector4F BasisX => GetColumn(0);
        public Vector4F BasisY => GetColumn(1);
        public Vector4F BasisZ => GetColumn(2);

        public float this[int rowIndex, int columnIndex]
        {
            [MethodImpl(MethodImplOptions.AggressiveInlining)]
            get
            {
                switch (rowIndex)
                {
                    case 0:
                        return row0[columnIndex];
                    case 1:
                        return row1[columnIndex];
                    case 2:
                        return row2[columnIndex];
                    case 3:
                        return row3[columnIndex];
                    default:
                        throw new IndexOutOfRangeException(nameof(rowIndex));
                }
            }
            [MethodImpl(MethodImplOptions.AggressiveInlining)]
            set
            {
                switch (rowIndex)
                {
                    case 0:
                        row0[columnIndex] = value;
                        break;
                    case 1:
                        row1[columnIndex] = value;
                        break;
                    case 2:
                        row2[columnIndex] = value;
                        break;
                    case 3:
                        row3[columnIndex] = value;
                        break;
                    default:
                        throw new IndexOutOfRangeException(nameof(rowIndex));
                }
            }
        }

        public Vector4F Row0
        {
            [MethodImpl(MethodImplOptions.AggressiveInlining)]
            get => row0;
            [MethodImpl(MethodImplOptions.AggressiveInlining)]
            set => row0 = value;
        }

        public Vector4F Row1
        {
            [MethodImpl(MethodImplOptions.AggressiveInlining)]
            get => row1;
            [MethodImpl(MethodImplOptions.AggressiveInlining)]
            set => row1 = value;
        }

        public Vector4F Row2
        {
            [MethodImpl(MethodImplOptions.AggressiveInlining)]
            get => row2;
            [MethodImpl(MethodImplOptions.AggressiveInlining)]
            set => row2 = value;
        }

        public Vector4F Row3
        {
            [MethodImpl(MethodImplOptions.AggressiveInlining)]
            get => row3;
            [MethodImpl(MethodImplOptions.AggressiveInlining)]
            set => row3 = value;
        }

        public Vector4F Column0
        {
            [MethodImpl(MethodImplOptions.AggressiveInlining)]
            get => GetColumn(0);
            [MethodImpl(MethodImplOptions.AggressiveInlining)]
            set => SetColumn(0, value);
        }

        public Vector4F Column1
        {
            [MethodImpl(MethodImplOptions.AggressiveInlining)]
            get => GetColumn(1);
            [MethodImpl(MethodImplOptions.AggressiveInlining)]
            set => SetColumn(1, value);
        }

        public Vector4F Column2
        {
            [MethodImpl(MethodImplOptions.AggressiveInlining)]
            get => GetColumn(2);
            [MethodImpl(MethodImplOptions.AggressiveInlining)]
            set => SetColumn(2, value);
        }

        public Vector4F Column3
        {
            [MethodImpl(MethodImplOptions.AggressiveInlining)]
            get => GetColumn(3);
            [MethodImpl(MethodImplOptions.AggressiveInlining)]
            set => SetColumn(3, value);
        }

        private Vector4F row0;
        private Vector4F row1;
        private Vector4F row2;
        private Vector4F row3;

        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public static Matrix4x4F FromRows(Vector4F row0, Vector4F row1, Vector4F row2, Vector4F row3)
        {
            return new Matrix4x4F
            {
                Row0 = row0,
                Row1 = row1,
                Row2 = row2,
                Row3 = row3
            };
        }

        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public static Matrix4x4F FromColumns(Vector4F column0, Vector4F column1, Vector4F column2, Vector4F column3)
        {
            return new Matrix4x4F
            {
                Column0 = column0,
                Column1 = column1,
                Column2 = column2,
                Column3 = column3
            };
        }

        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public static Matrix4x4F CreateRotationX(float angle)
        {
            var s = MathF.Sin(angle);
            var c = MathF.Cos(angle);
            return new Matrix4x4F
            {
                Row0 = Vector4F.UnitX,
                Row1 = new Vector4F(0.0f, c, -s, 0.0f),
                Row2 = new Vector4F(0.0f, s, +c, 0.0f),
                Row3 = Vector4F.UnitW
            };
        }

        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public static Matrix4x4F CreateRotationY(float angle)
        {
            var s = MathF.Sin(angle);
            var c = MathF.Cos(angle);
            return new Matrix4x4F
            {
                Row0 = new Vector4F(+c, 0.0f, s, 0.0f),
                Row1 = Vector4F.UnitY,
                Row2 = new Vector4F(-s, 0.0f, c, 0.0f),
                Row3 = Vector4F.UnitW
            };
        }

        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public static Matrix4x4F CreateRotationZ(float angle)
        {
            var s = MathF.Sin(angle);
            var c = MathF.Cos(angle);
            return new Matrix4x4F
            {
                Row0 = new Vector4F(c, -s, 0.0f, 0.0f),
                Row1 = new Vector4F(s, +c, 0.0f, 0.0f),
                Row2 = Vector4F.UnitZ,
                Row3 = Vector4F.UnitW
            };
        }

        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public static Matrix4x4F CreateScale(float x, float y, float z)
        {
            return CreateScale(new Vector3F(x, y, z));
        }

        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public static Matrix4x4F CreateScale(Vector3F scale)
        {
            return CreateDiagonal(new Vector4F(scale));
        }

        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public static Matrix4x4F CreateDiagonal(float x, float y, float z, float w)
        {
            return new Matrix4x4F
            {
                Row0 = Vector4F.UnitX * x,
                Row1 = Vector4F.UnitY * y,
                Row2 = Vector4F.UnitZ * z,
                Row3 = Vector4F.UnitW * w
            };
        }

        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public static Matrix4x4F CreateDiagonal(Vector4F diagonal)
        {
            return CreateDiagonal(diagonal.X, diagonal.Y, diagonal.Z, diagonal.W);
        }

        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public static Matrix4x4F CreateTranslation(float x, float y, float z)
        {
            return CreateTranslation(new Vector3F(x, y, z));
        }

        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public static Matrix4x4F CreateTranslation(Vector3F translation)
        {
            return new Matrix4x4F
            {
                Row0 = new Vector4F(1.0f, 0.0f, 0.0f, translation.X),
                Row1 = new Vector4F(0.0f, 1.0f, 0.0f, translation.Y),
                Row2 = new Vector4F(0.0f, 0.0f, 1.0f, translation.Z),
                Row3 = new Vector4F(0.0f, 0.0f, 0.0f, 1.0f)
            };
        }

        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public static Matrix4x4F CreateProjection(float fovY, float aspectRatio, float near, float far)
        {
            var cotY = MathF.Cos(0.5f * fovY) / MathF.Sin(0.5f * fovY);
            var cotX = cotY / aspectRatio;
            var invFl = 1.0f / (far - near);

            return new Matrix4x4F
            {
                Row0 = new Vector4F(-cotX, 0.0f, 0.0f, 0.0f),
                Row1 = new Vector4F(0.0f, cotY, 0.0f, 0.0f),
                Row2 = new Vector4F(0.0f, 0.0f, far * invFl, -far * near * invFl),
                Row3 = new Vector4F(0.0f, 0.0f, 1.0f, 0.0f)
            };
        }

        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public Vector4F GetRow(int index)
        {
            switch (index)
            {
                case 0:
                    return row0;
                case 1:
                    return row1;
                case 2:
                    return row2;
                case 3:
                    return row3;
                default:
                    throw new IndexOutOfRangeException(nameof(index));
            }
        }

        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public void SetRow(int index, Vector4F row)
        {
            switch (index)
            {
                case 0:
                    row0 = row;
                    break;
                case 1:
                    row1 = row;
                    break;
                case 2:
                    row2 = row;
                    break;
                case 3:
                    row3 = row;
                    break;
                default:
                    throw new IndexOutOfRangeException(nameof(index));
            }
        }

        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public void SetRow(int index, float x, float y, float z, float w)
        {
            SetRow(index, new Vector4F(x, y, z, w));
        }

        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public void SetRow(int index, Vector3F xyz, float w = 1f)
        {
            SetRow(index, new Vector4F(xyz, w));
        }

        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public Vector4F GetColumn(int index)
        {
            return new Vector4F(this[0, index], this[1, index], this[2, index], this[3, index]);
        }

        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public void SetColumn(int index, Vector4F column)
        {
            this[0, index] = column.X;
            this[1, index] = column.Y;
            this[2, index] = column.Z;
            this[3, index] = column.W;
        }

        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public void SetColumn(int index, float x, float y, float z, float w)
        {
            SetColumn(index, new Vector4F(x, y, z, w));
        }

        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public void SetColumn(int index, Vector3F xyz, float w = 1f)
        {
            SetColumn(index, new Vector4F(xyz, w));
        }

        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public float Determinant()
        {
            return DeterminantNative(ref this);
        }

        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public Matrix4x4F InverseTransform()
        {
            InverseTransformNative(ref this, out var result);
            return result;
        }

        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public static Matrix4x4F operator +(Matrix4x4F lhs, Matrix4x4F rhs)
        {
            var row0 = lhs.row0 + rhs.row0;
            var row1 = lhs.row1 + rhs.row1;
            var row2 = lhs.row2 + rhs.row2;
            var row3 = lhs.row3 + rhs.row3;
            return FromRows(row0, row1, row2, row3);
        }

        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public static Matrix4x4F operator -(Matrix4x4F lhs, Matrix4x4F rhs)
        {
            var row0 = lhs.row0 - rhs.row0;
            var row1 = lhs.row1 - rhs.row1;
            var row2 = lhs.row2 - rhs.row2;
            var row3 = lhs.row3 - rhs.row3;
            return FromRows(row0, row1, row2, row3);
        }

        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public static Matrix4x4F operator *(Matrix4x4F lhs, Matrix4x4F rhs)
        {
            MultiplyNative(ref lhs, ref rhs, out var result);
            return result;
        }

        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public static Vector4F operator *(Matrix4x4F lhs, Vector4F rhs)
        {
            MultiplyNative(ref lhs, ref rhs, out var result);
            return result;
        }

        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public static Matrix4x4F operator *(Matrix4x4F matrix, float f)
        {
            return new Matrix4x4F
            {
                Row0 = matrix.Row0 * f,
                Row1 = matrix.Row1 * f,
                Row2 = matrix.Row2 * f,
                Row3 = matrix.Row3 * f
            };
        }

        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public static Matrix4x4F operator /(Matrix4x4F matrix, float f)
        {
            return new Matrix4x4F
            {
                Row0 = matrix.Row0 / f,
                Row1 = matrix.Row1 / f,
                Row2 = matrix.Row2 / f,
                Row3 = matrix.Row3 / f
            };
        }

        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public static Matrix4x4F operator -(Matrix4x4F matrix)
        {
            return FromRows(-matrix.Row0, -matrix.Row1, -matrix.Row2, -matrix.Row3);
        }

        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public static bool AreApproxEqual(Matrix4x4F lhs, Matrix4x4F rhs, float epsilon = MathF.Epsilon)
        {
            return Vector4F.AreApproxEqual(lhs.Row0, rhs.Row0, epsilon)
                   && Vector4F.AreApproxEqual(lhs.Row1, rhs.Row1, epsilon)
                   && Vector4F.AreApproxEqual(lhs.Row2, rhs.Row2, epsilon)
                   && Vector4F.AreApproxEqual(lhs.Row3, rhs.Row3, epsilon);
        }

        public override string ToString()
        {
            return string.Join("; ", Enumerable.Range(0, 4).Select(GetRow));
        }

        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public bool Equals(Matrix4x4F other)
        {
            return row0.Equals(other.row0)
                   && row1.Equals(other.row1)
                   && row2.Equals(other.row2)
                   && row3.Equals(other.row3);
        }

        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public override bool Equals(object obj)
        {
            return obj is Matrix4x4F other && Equals(other);
        }

        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public static bool operator ==(Matrix4x4F lhs, Matrix4x4F rhs)
        {
            return lhs.Equals(rhs);
        }

        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public static bool operator !=(Matrix4x4F lhs, Matrix4x4F rhs)
        {
            return !lhs.Equals(rhs);
        }

        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public override int GetHashCode()
        {
            unchecked
            {
                var hashCode = row0.GetHashCode();
                hashCode = (hashCode * 397) ^ row1.GetHashCode();
                hashCode = (hashCode * 397) ^ row2.GetHashCode();
                hashCode = (hashCode * 397) ^ row3.GetHashCode();
                return hashCode;
            }
        }
    }
}
