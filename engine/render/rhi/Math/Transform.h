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

#include "Matrix3.h"
#include "BoundingSphere.h"

namespace Math {
// Orthonormal basis (just rotation via quaternion) and translation
class OrthogonalTransform;

// A 3x4 matrix that allows for asymmetric skew and scale
class AffineTransform;

// Uniform scale and translation that can be compactly represented in a vec4
class ScaleAndTranslation;

// Uniform scale, rotation (quaternion), and translation that fits in two vec4s
class UniformTransform;

// This transform strictly prohibits non-uniform scale.  Scale itself is barely tolerated.
class OrthogonalTransform {
 public:
  FE_FINLINE OrthogonalTransform() : m_rotation(kIdentity), m_translation(kZero) {}
  FE_FINLINE OrthogonalTransform(Quaternion rotate) : m_rotation(rotate), m_translation(kZero) {}
  FE_FINLINE OrthogonalTransform(Vector3 translate) : m_rotation(kIdentity), m_translation(translate) {}
  FE_FINLINE OrthogonalTransform(Quaternion rotate, Vector3 translate) : m_rotation(rotate), m_translation(translate) {}
  FE_FINLINE OrthogonalTransform(const Matrix3& mat) : m_rotation(mat), m_translation(kZero) {}
  FE_FINLINE OrthogonalTransform(const Matrix3& mat, Vector3 translate) : m_rotation(mat), m_translation(translate) {}
  FE_FINLINE OrthogonalTransform(EIdentityTag) : m_rotation(kIdentity), m_translation(kZero) {}
  FE_FINLINE explicit OrthogonalTransform(const XMMATRIX& mat) { *this = OrthogonalTransform(Matrix3(mat), Vector3(mat.r[3])); }

  FE_FINLINE void SetRotation(Quaternion q) { m_rotation = q; }
  FE_FINLINE void SetTranslation(Vector3 v) { m_translation = v; }

  FE_FINLINE Quaternion GetRotation() const { return m_rotation; }
  FE_FINLINE Vector3 GetTranslation() const { return m_translation; }

  static FE_FINLINE OrthogonalTransform MakeXRotation(float angle) { return OrthogonalTransform(Quaternion(Vector3(kXUnitVector), angle)); }
  static FE_FINLINE OrthogonalTransform MakeYRotation(float angle) { return OrthogonalTransform(Quaternion(Vector3(kYUnitVector), angle)); }
  static FE_FINLINE OrthogonalTransform MakeZRotation(float angle) { return OrthogonalTransform(Quaternion(Vector3(kZUnitVector), angle)); }
  static FE_FINLINE OrthogonalTransform MakeTranslation(Vector3 translate) { return OrthogonalTransform(translate); }

  FE_FINLINE Vector3 operator*(Vector3 vec) const { return m_rotation * vec + m_translation; }
  FE_FINLINE Vector4 operator*(Vector4 vec) const {
    return Vector4(SetWToZero(m_rotation * Vector3((XMVECTOR)vec))) + Vector4(SetWToOne(m_translation)) * vec.GetW();
  }
  FE_FINLINE BoundingSphere operator*(BoundingSphere sphere) const { return BoundingSphere(*this * sphere.GetCenter(), sphere.GetRadius()); }

  FE_FINLINE OrthogonalTransform operator*(const OrthogonalTransform& xform) const {
    return OrthogonalTransform(m_rotation * xform.m_rotation, m_rotation * xform.m_translation + m_translation);
  }

  FE_FINLINE OrthogonalTransform operator~() const {
    Quaternion invertedRotation = ~m_rotation;
    return OrthogonalTransform(invertedRotation, invertedRotation * -m_translation);
  }

 private:
  Quaternion m_rotation;
  Vector3 m_translation;
};

//
// A transform that lacks rotation and has only uniform scale.
//
class ScaleAndTranslation {
 public:
  FE_FINLINE ScaleAndTranslation() {}
  FE_FINLINE ScaleAndTranslation(EIdentityTag) : m_repr(kWUnitVector) {}
  FE_FINLINE ScaleAndTranslation(float tx, float ty, float tz, float scale) : m_repr(tx, ty, tz, scale) {}
  FE_FINLINE ScaleAndTranslation(Vector3 translate, Scalar scale) {
    m_repr = Vector4(translate);
    m_repr.SetW(scale);
  }
  FE_FINLINE explicit ScaleAndTranslation(const XMVECTOR& v) : m_repr(v) {}

  FE_FINLINE void SetScale(Scalar s) { m_repr.SetW(s); }
  FE_FINLINE void SetTranslation(Vector3 t) { m_repr.SetXYZ(t); }

  FE_FINLINE Scalar GetScale() const { return m_repr.GetW(); }
  FE_FINLINE Vector3 GetTranslation() const { return (Vector3)m_repr; }

  FE_FINLINE BoundingSphere operator*(const BoundingSphere& sphere) const {
    Vector4 scaledSphere = (Vector4)sphere * GetScale();
    Vector4 translation = Vector4(SetWToZero(m_repr));
    return BoundingSphere(scaledSphere + translation);
  }

 private:
  Vector4 m_repr;
};

//
// This transform allows for rotation, translation, and uniform scale
//
class UniformTransform {
 public:
  FE_FINLINE UniformTransform() {}
  FE_FINLINE UniformTransform(EIdentityTag) : m_rotation(kIdentity), m_translationScale(kIdentity) {}
  FE_FINLINE UniformTransform(Quaternion rotation, ScaleAndTranslation transScale) : m_rotation(rotation), m_translationScale(transScale) {}
  FE_FINLINE UniformTransform(Quaternion rotation, Scalar scale, Vector3 translation)
      : m_rotation(rotation), m_translationScale(translation, scale) {}

  FE_FINLINE void SetRotation(Quaternion r) { m_rotation = r; }
  FE_FINLINE void SetScale(Scalar s) { m_translationScale.SetScale(s); }
  FE_FINLINE void SetTranslation(Vector3 t) { m_translationScale.SetTranslation(t); }

  FE_FINLINE Quaternion GetRotation() const { return m_rotation; }
  FE_FINLINE Scalar GetScale() const { return m_translationScale.GetScale(); }
  FE_FINLINE Vector3 GetTranslation() const { return m_translationScale.GetTranslation(); }

  FE_FINLINE Vector3 operator*(Vector3 vec) const { return m_rotation * (vec * m_translationScale.GetScale()) + m_translationScale.GetTranslation(); }

  FE_FINLINE BoundingSphere operator*(BoundingSphere sphere) const {
    return BoundingSphere(*this * sphere.GetCenter(), GetScale() * sphere.GetRadius());
  }

 private:
  Quaternion m_rotation;
  ScaleAndTranslation m_translationScale;
};

// A AffineTransform is a 3x4 matrix with an implicit 4th row = [0,0,0,1].  This is used to perform a change of
// basis on 3D points.  An affine transformation does not have to have orthonormal basis vectors.
class AffineTransform {
 public:
  FE_FINLINE AffineTransform() {}
  FE_FINLINE AffineTransform(Vector3 x, Vector3 y, Vector3 z, Vector3 w) : m_basis(x, y, z), m_translation(w) {}
  FE_FINLINE AffineTransform(Vector3 translate) : m_basis(kIdentity), m_translation(translate) {}
  FE_FINLINE AffineTransform(const Matrix3& mat, Vector3 translate = Vector3(kZero)) : m_basis(mat), m_translation(translate) {}
  FE_FINLINE AffineTransform(Quaternion rot, Vector3 translate = Vector3(kZero)) : m_basis(rot), m_translation(translate) {}
  FE_FINLINE AffineTransform(const OrthogonalTransform& xform) : m_basis(xform.GetRotation()), m_translation(xform.GetTranslation()) {}
  FE_FINLINE AffineTransform(const UniformTransform& xform) {
    m_basis = Matrix3(xform.GetRotation()) * xform.GetScale();
    m_translation = xform.GetTranslation();
  }
  FE_FINLINE AffineTransform(EIdentityTag) : m_basis(kIdentity), m_translation(kZero) {}
  FE_FINLINE explicit AffineTransform(const XMMATRIX& mat) : m_basis(mat), m_translation(mat.r[3]) {}

  FE_FINLINE operator XMMATRIX() const { return (XMMATRIX&)*this; }

  FE_FINLINE void SetX(Vector3 x) { m_basis.SetX(x); }
  FE_FINLINE void SetY(Vector3 y) { m_basis.SetY(y); }
  FE_FINLINE void SetZ(Vector3 z) { m_basis.SetZ(z); }
  FE_FINLINE void SetTranslation(Vector3 w) { m_translation = w; }
  FE_FINLINE void SetBasis(const Matrix3& basis) { m_basis = basis; }

  FE_FINLINE Vector3 GetX() const { return m_basis.GetX(); }
  FE_FINLINE Vector3 GetY() const { return m_basis.GetY(); }
  FE_FINLINE Vector3 GetZ() const { return m_basis.GetZ(); }
  FE_FINLINE Vector3 GetTranslation() const { return m_translation; }
  FE_FINLINE const Matrix3& GetBasis() const { return (const Matrix3&)*this; }

  static FE_FINLINE AffineTransform MakeXRotation(float angle) { return AffineTransform(Matrix3::MakeXRotation(angle)); }
  static FE_FINLINE AffineTransform MakeYRotation(float angle) { return AffineTransform(Matrix3::MakeYRotation(angle)); }
  static FE_FINLINE AffineTransform MakeZRotation(float angle) { return AffineTransform(Matrix3::MakeZRotation(angle)); }
  static FE_FINLINE AffineTransform MakeScale(float scale) { return AffineTransform(Matrix3::MakeScale(scale)); }
  static FE_FINLINE AffineTransform MakeScale(Vector3 scale) { return AffineTransform(Matrix3::MakeScale(scale)); }
  static FE_FINLINE AffineTransform MakeTranslation(Vector3 translate) { return AffineTransform(translate); }

  FE_FINLINE Vector3 operator*(Vector3 vec) const { return m_basis * vec + m_translation; }
  FE_FINLINE AffineTransform operator*(const AffineTransform& mat) const {
    return AffineTransform(m_basis * mat.m_basis, *this * mat.GetTranslation());
  }

 private:
  Matrix3 m_basis;
  Vector3 m_translation;
};
}  // namespace Math
