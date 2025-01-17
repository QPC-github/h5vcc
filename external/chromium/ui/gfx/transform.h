// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UI_GFX_TRANSFORM_H_
#define UI_GFX_TRANSFORM_H_

#include "base/compiler_specific.h"
#include "third_party/skia/include/utils/SkMatrix44.h"
#include "ui/base/ui_export.h"

namespace gfx {

class RectF;
class Point;
class Point3F;
class Vector3dF;

// 4x4 transformation matrix. Transform is cheap and explicitly allows
// copy/assign.
class UI_EXPORT Transform {
 public:

  enum SkipInitialization {
    kSkipInitialization
  };

  Transform() : matrix_(SkMatrix44::kIdentity_Constructor) {}

  // Skips initializing this matrix to avoid overhead, when we know it will be
  // initialized before use.
  Transform(SkipInitialization)
      : matrix_(SkMatrix44::kUninitialized_Constructor) {}
  Transform(const Transform& rhs) : matrix_(rhs.matrix_) {}
  // Initialize with the concatenation of lhs * rhs.
  Transform(const Transform& lhs, const Transform& rhs)
      : matrix_(lhs.matrix_, rhs.matrix_) {}
  ~Transform() {}

  bool operator==(const Transform& rhs) const { return matrix_ == rhs.matrix_; }
  bool operator!=(const Transform& rhs) const { return matrix_ != rhs.matrix_; }

  // Resets this transform to the identity transform.
  void MakeIdentity() { matrix_.setIdentity(); }

  // Applies the current transformation on a 2d rotation and assigns the result
  // to |this|.
  void Rotate(double degrees) { RotateAboutZAxis(degrees); }

  // Applies the current transformation on an axis-angle rotation and assigns
  // the result to |this|.
  void RotateAboutXAxis(double degrees);
  void RotateAboutYAxis(double degrees);
  void RotateAboutZAxis(double degrees);
  void RotateAbout(const Vector3dF& axis, double degrees);

  // Applies the current transformation on a scaling and assigns the result
  // to |this|.
  void Scale(double x, double y);
  void Scale3d(double x, double y, double z);

  // Applies the current transformation on a translation and assigns the result
  // to |this|.
  void Translate(double x, double y);
  void Translate3d(double x, double y, double z);

  // Applies the current transformation on a skew and assigns the result
  // to |this|.
  void SkewX(double angle_x);
  void SkewY(double angle_y);

  // Applies the current transformation on a perspective transform and assigns
  // the result to |this|.
  void ApplyPerspectiveDepth(double depth);

  // Applies a transformation on the current transformation
  // (i.e. 'this = this * transform;').
  void PreconcatTransform(const Transform& transform);

  // Applies a transformation on the current transformation
  // (i.e. 'this = transform * this;').
  void ConcatTransform(const Transform& transform);

  // Returns true if this is the identity matrix.
  bool IsIdentity() const { return matrix_.isIdentity(); }

  // Returns true if the matrix is either identity or pure translation.
  bool IsIdentityOrTranslation() const {
    return !(matrix_.getType() & ~SkMatrix44::kTranslate_Mask);
  }

  // Returns true if the matrix is either identity or pure, non-fractional
  // translation.
  bool IsIdentityOrIntegerTranslation() const;

  // Returns true if the matrix is has only scaling and translation components.
  bool IsScaleOrTranslation() const {
    int mask = SkMatrix44::kScale_Mask | SkMatrix44::kTranslate_Mask;
    return (matrix_.getType() & ~mask) == 0;
  }

  // Returns true if the matrix has any perspective component that would
  // change the w-component of a homogeneous point.
  bool HasPerspective() const {
    return (matrix_.getType() & SkMatrix44::kPerspective_Mask) != 0;
  }

  // Returns true if this transform is non-singular.
  bool IsInvertible() const { return matrix_.invert(NULL); }

  // Returns true if a layer with a forward-facing normal of (0, 0, 1) would
  // have its back side facing frontwards after applying the transform.
  bool IsBackFaceVisible() const;

  // Inverts the transform which is passed in. Returns true if successful.
  bool GetInverse(Transform* transform) const WARN_UNUSED_RESULT;

  // Transposes this transform in place.
  void Transpose();

  // Applies the transformation on the point. Returns true if the point is
  // transformed successfully.
  void TransformPoint(Point3F& point) const;

  // Applies the transformation on the point. Returns true if the point is
  // transformed successfully. Rounds the result to the nearest point.
  void TransformPoint(Point& point) const;

  // Applies the reverse transformation on the point. Returns true if the
  // transformation can be inverted.
  bool TransformPointReverse(Point3F& point) const;

  // Applies the reverse transformation on the point. Returns true if the
  // transformation can be inverted. Rounds the result to the nearest point.
  bool TransformPointReverse(Point& point) const;

  // Applies transformation on the rectangle. Returns true if the transformed
  // rectangle was axis aligned. If it returns false, rect will be the
  // smallest axis aligned bounding box containing the transformed rect.
  void TransformRect(RectF* rect) const;

  // Applies the reverse transformation on the rectangle. Returns true if
  // the transformed rectangle was axis aligned. If it returns false,
  // rect will be the smallest axis aligned bounding box containing the
  // transformed rect.
  bool TransformRectReverse(RectF* rect) const;

  // Decomposes |this| and |from|, interpolates the decomposed values, and
  // sets |this| to the reconstituted result. Returns false if either matrix
  // can't be decomposed. Uses routines described in this spec:
  // http://www.w3.org/TR/css3-3d-transforms/.
  //
  // Note: this call is expensive since we need to decompose the transform. If
  // you're going to be calling this rapidly (e.g., in an animation) you should
  // decompose once using gfx::DecomposeTransforms and reuse your
  // DecomposedTransform.
  bool Blend(const Transform& from, double progress);

  // Returns |this| * |other|.
  Transform operator*(const Transform& other) const {
    return Transform(*this, other);
  }

  // Sets |this| = |this| * |other|
  Transform& operator*=(const Transform& other) {
    PreconcatTransform(other);
    return *this;
  }

  // Returns the underlying matrix.
  const SkMatrix44& matrix() const { return matrix_; }
  SkMatrix44& matrix() { return matrix_; }

 private:
  void TransformPointInternal(const SkMatrix44& xform,
                              Point& point) const;

  void TransformPointInternal(const SkMatrix44& xform,
                              Point3F& point) const;

  SkMatrix44 matrix_;

  // copy/assign are allowed.
};

}  // namespace gfx

#endif  // UI_GFX_TRANSFORM_H_
