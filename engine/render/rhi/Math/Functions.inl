//
// Copyright (c) Microsoft. All rights reserved.
// This code is licensed under the MIT License (MIT).
// THIS CODE IS PROVIDED *AS IS* WITHOUT WARRANTY OF
// ANY KIND, EITHER EXPRESS OR IMPLIED, INCLUDING ANY
// IMPLIED WARRANTIES OF FITNESS FOR A PARTICULAR
// PURPOSE, MERCHANTABILITY, OR NON-INFRINGEMENT.
//
// Developed by Minigraph
//
// Author:  James Stanard
//

#pragma once

namespace Math {
// To allow floats to implicitly construct Scalars, we need to clarify these operators and suppress
// upconversion.
FE_FINLINE bool operator<(Scalar lhs, float rhs) {
  return (float)lhs < rhs;
}
FE_FINLINE bool operator<=(Scalar lhs, float rhs) {
  return (float)lhs <= rhs;
}
FE_FINLINE bool operator>(Scalar lhs, float rhs) {
  return (float)lhs > rhs;
}
FE_FINLINE bool operator>=(Scalar lhs, float rhs) {
  return (float)lhs >= rhs;
}
FE_FINLINE bool operator==(Scalar lhs, float rhs) {
  return (float)lhs == rhs;
}
FE_FINLINE bool operator<(float lhs, Scalar rhs) {
  return lhs < (float)rhs;
}
FE_FINLINE bool operator<=(float lhs, Scalar rhs) {
  return lhs <= (float)rhs;
}
FE_FINLINE bool operator>(float lhs, Scalar rhs) {
  return lhs > (float)rhs;
}
FE_FINLINE bool operator>=(float lhs, Scalar rhs) {
  return lhs >= (float)rhs;
}
FE_FINLINE bool operator==(float lhs, Scalar rhs) {
  return lhs == (float)rhs;
}

#define CREATE_SIMD_FUNCTIONS(TYPE)                             \
  FE_FINLINE TYPE Sqrt(TYPE s) {                                \
    return TYPE(XMVectorSqrt(s));                               \
  }                                                             \
  FE_FINLINE TYPE Recip(TYPE s) {                               \
    return TYPE(XMVectorReciprocal(s));                         \
  }                                                             \
  FE_FINLINE TYPE RecipSqrt(TYPE s) {                           \
    return TYPE(XMVectorReciprocalSqrt(s));                     \
  }                                                             \
  FE_FINLINE TYPE Floor(TYPE s) {                               \
    return TYPE(XMVectorFloor(s));                              \
  }                                                             \
  FE_FINLINE TYPE Ceiling(TYPE s) {                             \
    return TYPE(XMVectorCeiling(s));                            \
  }                                                             \
  FE_FINLINE TYPE Round(TYPE s) {                               \
    return TYPE(XMVectorRound(s));                              \
  }                                                             \
  FE_FINLINE TYPE Abs(TYPE s) {                                 \
    return TYPE(XMVectorAbs(s));                                \
  }                                                             \
  FE_FINLINE TYPE Exp(TYPE s) {                                 \
    return TYPE(XMVectorExp(s));                                \
  }                                                             \
  FE_FINLINE TYPE Pow(TYPE b, TYPE e) {                         \
    return TYPE(XMVectorPow(b, e));                             \
  }                                                             \
  FE_FINLINE TYPE Log(TYPE s) {                                 \
    return TYPE(XMVectorLog(s));                                \
  }                                                             \
  FE_FINLINE TYPE Sin(TYPE s) {                                 \
    return TYPE(XMVectorSin(s));                                \
  }                                                             \
  FE_FINLINE TYPE Cos(TYPE s) {                                 \
    return TYPE(XMVectorCos(s));                                \
  }                                                             \
  FE_FINLINE TYPE Tan(TYPE s) {                                 \
    return TYPE(XMVectorTan(s));                                \
  }                                                             \
  FE_FINLINE TYPE ASin(TYPE s) {                                \
    return TYPE(XMVectorASin(s));                               \
  }                                                             \
  FE_FINLINE TYPE ACos(TYPE s) {                                \
    return TYPE(XMVectorACos(s));                               \
  }                                                             \
  FE_FINLINE TYPE ATan(TYPE s) {                                \
    return TYPE(XMVectorATan(s));                               \
  }                                                             \
  FE_FINLINE TYPE ATan2(TYPE y, TYPE x) {                       \
    return TYPE(XMVectorATan2(y, x));                           \
  }                                                             \
  FE_FINLINE TYPE Lerp(TYPE a, TYPE b, TYPE t) {                \
    return TYPE(XMVectorLerpV(a, b, t));                        \
  }                                                             \
  FE_FINLINE TYPE Lerp(TYPE a, TYPE b, float t) {               \
    return TYPE(XMVectorLerp(a, b, t));                         \
  }                                                             \
  FE_FINLINE TYPE Max(TYPE a, TYPE b) {                         \
    return TYPE(XMVectorMax(a, b));                             \
  }                                                             \
  FE_FINLINE TYPE Min(TYPE a, TYPE b) {                         \
    return TYPE(XMVectorMin(a, b));                             \
  }                                                             \
  FE_FINLINE TYPE Clamp(TYPE v, TYPE a, TYPE b) {               \
    return Min(Max(v, a), b);                                   \
  }                                                             \
  FE_FINLINE BoolVector operator<(TYPE lhs, TYPE rhs) {         \
    return XMVectorLess(lhs, rhs);                              \
  }                                                             \
  FE_FINLINE BoolVector operator<=(TYPE lhs, TYPE rhs) {        \
    return XMVectorLessOrEqual(lhs, rhs);                       \
  }                                                             \
  FE_FINLINE BoolVector operator>(TYPE lhs, TYPE rhs) {         \
    return XMVectorGreater(lhs, rhs);                           \
  }                                                             \
  FE_FINLINE BoolVector operator>=(TYPE lhs, TYPE rhs) {        \
    return XMVectorGreaterOrEqual(lhs, rhs);                    \
  }                                                             \
  FE_FINLINE BoolVector operator==(TYPE lhs, TYPE rhs) {        \
    return XMVectorEqual(lhs, rhs);                             \
  }                                                             \
  FE_FINLINE TYPE Select(TYPE lhs, TYPE rhs, BoolVector mask) { \
    return TYPE(XMVectorSelect(lhs, rhs, mask));                \
  }

CREATE_SIMD_FUNCTIONS(Scalar)
CREATE_SIMD_FUNCTIONS(Vector3)
CREATE_SIMD_FUNCTIONS(Vector4)

#undef CREATE_SIMD_FUNCTIONS

FE_FINLINE float Sqrt(float s) {
  return Sqrt(Scalar(s));
}
FE_FINLINE float Recip(float s) {
  return Recip(Scalar(s));
}
FE_FINLINE float RecipSqrt(float s) {
  return RecipSqrt(Scalar(s));
}
FE_FINLINE float Floor(float s) {
  return Floor(Scalar(s));
}
FE_FINLINE float Ceiling(float s) {
  return Ceiling(Scalar(s));
}
FE_FINLINE float Round(float s) {
  return Round(Scalar(s));
}
FE_FINLINE float Abs(float s) {
  return s < 0.0f ? -s : s;
}
FE_FINLINE float Exp(float s) {
  return Exp(Scalar(s));
}
FE_FINLINE float Pow(float b, float e) {
  return Pow(Scalar(b), Scalar(e));
}
FE_FINLINE float Log(float s) {
  return Log(Scalar(s));
}
FE_FINLINE float Sin(float s) {
  return Sin(Scalar(s));
}
FE_FINLINE float Cos(float s) {
  return Cos(Scalar(s));
}
FE_FINLINE float Tan(float s) {
  return Tan(Scalar(s));
}
FE_FINLINE float ASin(float s) {
  return ASin(Scalar(s));
}
FE_FINLINE float ACos(float s) {
  return ACos(Scalar(s));
}
FE_FINLINE float ATan(float s) {
  return ATan(Scalar(s));
}
FE_FINLINE float ATan2(float y, float x) {
  return ATan2(Scalar(y), Scalar(x));
}
FE_FINLINE float Lerp(float a, float b, float t) {
  return a + (b - a) * t;
}
FE_FINLINE float Max(float a, float b) {
  return a > b ? a : b;
}
FE_FINLINE float Min(float a, float b) {
  return a < b ? a : b;
}
FE_FINLINE float Clamp(float v, float a, float b) {
  return Min(Max(v, a), b);
}

FE_FINLINE Scalar Length(Vector3 v) {
  return Scalar(XMVector3Length(v));
}
FE_FINLINE Scalar LengthSquare(Vector3 v) {
  return Scalar(XMVector3LengthSq(v));
}
FE_FINLINE Scalar LengthRecip(Vector3 v) {
  return Scalar(XMVector3ReciprocalLength(v));
}
FE_FINLINE Scalar Dot(Vector3 v1, Vector3 v2) {
  return Scalar(XMVector3Dot(v1, v2));
}
FE_FINLINE Scalar Dot(Vector4 v1, Vector4 v2) {
  return Scalar(XMVector4Dot(v1, v2));
}
FE_FINLINE Vector3 Cross(Vector3 v1, Vector3 v2) {
  return Vector3(XMVector3Cross(v1, v2));
}
FE_FINLINE Vector3 Normalize(Vector3 v) {
  return Vector3(XMVector3Normalize(v));
}
FE_FINLINE Vector4 Normalize(Vector4 v) {
  return Vector4(XMVector4Normalize(v));
}

FE_FINLINE Matrix3 Transpose(const Matrix3& mat) {
  return Matrix3(XMMatrixTranspose(mat));
}
FE_FINLINE Matrix3 InverseTranspose(const Matrix3& mat) {
  const Vector3 x = mat.GetX();
  const Vector3 y = mat.GetY();
  const Vector3 z = mat.GetZ();

  const Vector3 inv0 = Cross(y, z);
  const Vector3 inv1 = Cross(z, x);
  const Vector3 inv2 = Cross(x, y);
  const Scalar rDet = Recip(Dot(z, inv2));

  // Return the adjoint / determinant
  return Matrix3(inv0, inv1, inv2) * rDet;
}

// FE_FINLINE Matrix3 Inverse( const Matrix3& mat ) { TBD }
// FE_FINLINE Transform Inverse( const Transform& mat ) { TBD }

// This specialized matrix invert assumes that the 3x3 matrix is orthogonal (and normalized).
FE_FINLINE AffineTransform OrthoInvert(const AffineTransform& xform) {
  Matrix3 basis = Transpose(xform.GetBasis());
  return AffineTransform(basis, basis * -xform.GetTranslation());
}

FE_FINLINE OrthogonalTransform Invert(const OrthogonalTransform& xform) {
  return ~xform;
}

FE_FINLINE Matrix4 Transpose(const Matrix4& mat) {
  return Matrix4(XMMatrixTranspose(mat));
}
FE_FINLINE Matrix4 Invert(const Matrix4& mat) {
  return Matrix4(XMMatrixInverse(nullptr, mat));
}

FE_FINLINE Matrix4 OrthoInvert(const Matrix4& xform) {
  Matrix3 basis = Transpose(xform.Get3x3());
  Vector3 translate = basis * -Vector3(xform.GetW());
  return Matrix4(basis, translate);
}

}  // namespace Math