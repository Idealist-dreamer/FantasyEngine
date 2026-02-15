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

#include "Common.h"

namespace Math {
class Scalar {
 public:
  FE_FINLINE Scalar() {}
  FE_FINLINE Scalar(const Scalar& s) { m_vec = s; }
  FE_FINLINE Scalar(float f) { m_vec = XMVectorReplicate(f); }
  FE_FINLINE explicit Scalar(FXMVECTOR vec) { m_vec = vec; }
  FE_FINLINE explicit Scalar(EZeroTag) { m_vec = SplatZero(); }
  FE_FINLINE explicit Scalar(EIdentityTag) { m_vec = SplatOne(); }

  FE_FINLINE operator XMVECTOR() const { return m_vec; }
  FE_FINLINE operator float() const { return XMVectorGetX(m_vec); }

 private:
  XMVECTOR m_vec;
};

FE_FINLINE Scalar operator-(Scalar s) {
  return Scalar(XMVectorNegate(s));
}
FE_FINLINE Scalar operator+(Scalar s1, Scalar s2) {
  return Scalar(XMVectorAdd(s1, s2));
}
FE_FINLINE Scalar operator-(Scalar s1, Scalar s2) {
  return Scalar(XMVectorSubtract(s1, s2));
}
FE_FINLINE Scalar operator*(Scalar s1, Scalar s2) {
  return Scalar(XMVectorMultiply(s1, s2));
}
FE_FINLINE Scalar operator/(Scalar s1, Scalar s2) {
  return Scalar(XMVectorDivide(s1, s2));
}
FE_FINLINE Scalar operator+(Scalar s1, float s2) {
  return s1 + Scalar(s2);
}
FE_FINLINE Scalar operator-(Scalar s1, float s2) {
  return s1 - Scalar(s2);
}
FE_FINLINE Scalar operator*(Scalar s1, float s2) {
  return s1 * Scalar(s2);
}
FE_FINLINE Scalar operator/(Scalar s1, float s2) {
  return s1 / Scalar(s2);
}
FE_FINLINE Scalar operator+(float s1, Scalar s2) {
  return Scalar(s1) + s2;
}
FE_FINLINE Scalar operator-(float s1, Scalar s2) {
  return Scalar(s1) - s2;
}
FE_FINLINE Scalar operator*(float s1, Scalar s2) {
  return Scalar(s1) * s2;
}
FE_FINLINE Scalar operator/(float s1, Scalar s2) {
  return Scalar(s1) / s2;
}

}  // namespace Math
