/*
   Copyright (C) 2006, 2007 Sony Computer Entertainment Inc.
   All rights reserved.

   Redistribution and use in source and binary forms,
   with or without modification, are permitted provided that the
   following conditions are met:
    * Redistributions of source code must retain the above copyright
      notice, this list of conditions and the following disclaimer.
    * Redistributions in binary form must reproduce the above copyright
      notice, this list of conditions and the following disclaimer in the
      documentation and/or other materials provided with the distribution.
    * Neither the name of the Sony Computer Entertainment Inc nor the names
      of its contributors may be used to endorse or promote products derived
      from this software without specific prior written permission.

   THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
   AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
   IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
   ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
   LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
   CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
   SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
   INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
   CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
   ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
   POSSIBILITY OF SUCH DAMAGE.
*/

#ifndef VECTORMATH_SSE_VECTOR_HPP
#define VECTORMATH_SSE_VECTOR_HPP

namespace Vectormath
{
namespace SSE
{

// ========================================================
// VecIdx
// ========================================================

#ifdef VECTORMATH_NO_SCALAR_CAST
inline VecIdx::operator FloatInVec() const
{
    return FloatInVec(ref, i);
}
inline float VecIdx::getAsFloat() const
#else
inline VecIdx::operator float() const
#endif
{
    return ((float *)&ref)[i];
}

inline float VecIdx::operator = (float scalar)
{
    sseVecSetElement(ref, scalar, i);
    return scalar;
}

inline FloatInVec VecIdx::operator = (const FloatInVec & scalar)
{
    ref = sseVecInsert(ref, scalar.get128(), i);
    return scalar;
}

inline FloatInVec VecIdx::operator = (const VecIdx & scalar)
{
    return *this = FloatInVec(scalar.ref, scalar.i);
}

inline FloatInVec VecIdx::operator *= (float scalar)
{
    return *this *= FloatInVec(scalar);
}

inline FloatInVec VecIdx::operator *= (const FloatInVec & scalar)
{
    return *this = FloatInVec(ref, i) * scalar;
}

inline FloatInVec VecIdx::operator /= (float scalar)
{
    return *this /= FloatInVec(scalar);
}

inline FloatInVec VecIdx::operator /= (const FloatInVec & scalar)
{
    return *this = FloatInVec(ref, i) / scalar;
}

inline FloatInVec VecIdx::operator += (float scalar)
{
    return *this += FloatInVec(scalar);
}

inline FloatInVec VecIdx::operator += (const FloatInVec & scalar)
{
    return *this = FloatInVec(ref, i) + scalar;
}

inline FloatInVec VecIdx::operator -= (float scalar)
{
    return *this -= FloatInVec(scalar);
}

inline FloatInVec VecIdx::operator -= (const FloatInVec & scalar)
{
    return *this = FloatInVec(ref, i) - scalar;
}

// ========================================================
// VecIdxd
// ========================================================

#ifdef VECTORMATH_NO_SCALAR_CAST
inline VecIdxd::operator DoubleInVec() const
{
	return DoubleInVec(ref, i);
}
inline double VecIdxd::getAsDouble() const
#else
inline VecIdxd::operator double() const
#endif
{
	return ((double *)&ref)[i];
}

inline double VecIdxd::operator = (double scalar)
{
	dsseVecSetElement(ref, scalar, i);
	return scalar;
}

inline DoubleInVec VecIdxd::operator = (const DoubleInVec & scalar)
{
	ref = dsseVecInsert(ref, scalar.get256(), i);
	return scalar;
}

inline DoubleInVec VecIdxd::operator = (const VecIdxd & scalar)
{
	return *this = DoubleInVec(scalar.ref, scalar.i);
}

inline DoubleInVec VecIdxd::operator *= (double scalar)
{
	return *this *= DoubleInVec(scalar);
}

inline DoubleInVec VecIdxd::operator *= (const DoubleInVec & scalar)
{
	return *this = DoubleInVec(ref, i) * scalar;
}

inline DoubleInVec VecIdxd::operator /= (double scalar)
{
	return *this /= DoubleInVec(scalar);
}

inline DoubleInVec VecIdxd::operator /= (const DoubleInVec & scalar)
{
	return *this = DoubleInVec(ref, i) / scalar;
}

inline DoubleInVec VecIdxd::operator += (double scalar)
{
	return *this += DoubleInVec(scalar);
}

inline DoubleInVec VecIdxd::operator += (const DoubleInVec & scalar)
{
	return *this = DoubleInVec(ref, i) + scalar;
}

inline DoubleInVec VecIdxd::operator -= (double scalar)
{
	return *this -= DoubleInVec(scalar);
}

inline DoubleInVec VecIdxd::operator -= (const DoubleInVec & scalar)
{
	return *this = DoubleInVec(ref, i) - scalar;
}

// ========================================================
// Vector3
// ========================================================

inline Vector3::Vector3(float _x, float _y, float _z)
{
    mVec128 = _mm_setr_ps(_x, _y, _z, 0.0f);
}

inline Vector3::Vector3(const FloatInVec & _x, const FloatInVec & _y, const FloatInVec & _z)
{
    const __m128 xz = _mm_unpacklo_ps(_x.get128(), _z.get128());
    mVec128 = _mm_unpacklo_ps(xz, _y.get128());
}

inline Vector3::Vector3(const Point3 & pnt)
{
    mVec128 = pnt.get128();
    setW(0);
}

inline Vector3::Vector3(float scalar): Vector3(FloatInVec(scalar).get128())
{
}

inline Vector3::Vector3(const FloatInVec & scalar): Vector3(scalar.get128())
{
}

inline Vector3::Vector3(__m128 vf4)
{
    mVec128 = vf4;
    setW(0);
}

inline const Vector3 Vector3::xAxis()
{
    return Vector3(sseUnitVec1000());
}

inline const Vector3 Vector3::yAxis()
{
    return Vector3(sseUnitVec0100());
}

inline const Vector3 Vector3::zAxis()
{
    return Vector3(sseUnitVec0010());
}

inline const Vector3 lerp(float t, const Vector3 & vec0, const Vector3 & vec1)
{
    return lerp(FloatInVec(t), vec0, vec1);
}

inline const Vector3 lerp(const FloatInVec & t, const Vector3 & vec0, const Vector3 & vec1)
{
    return (vec0 + ((vec1 - vec0) * t));
}

inline const Vector3 slerp(float t, const Vector3 & unitVec0, const Vector3 & unitVec1)
{
    return slerp(FloatInVec(t), unitVec0, unitVec1);
}

inline const Vector3 slerp(const FloatInVec & t, const Vector3 & unitVec0, const Vector3 & unitVec1)
{
    __m128 scales, scale0, scale1, cosAngle, angle, tttt, oneMinusT, angles, sines;
    cosAngle = sseVecDot3(unitVec0.get128(), unitVec1.get128());
    __m128 selectMask = _mm_cmpgt_ps(_mm_set1_ps(VECTORMATH_SLERP_TOL), cosAngle);
    angle = sseACosf(cosAngle);
    tttt = t.get128();
    oneMinusT = _mm_sub_ps(_mm_set1_ps(1.0f), tttt);
    angles = _mm_unpacklo_ps(_mm_set1_ps(1.0f), tttt); // angles = 1, t, 1, t
    angles = _mm_unpacklo_ps(angles, oneMinusT);       // angles = 1, 1-t, t, 1-t
    angles = _mm_mul_ps(angles, angle);
    sines = sseSinf(angles);
    scales = _mm_div_ps(sines, sseSplat(sines, 0));
    scale0 = sseSelect(oneMinusT, sseSplat(scales, 1), selectMask);
    scale1 = sseSelect(tttt, sseSplat(scales, 2), selectMask);
    return Vector3(sseMAdd(unitVec0.get128(), scale0, _mm_mul_ps(unitVec1.get128(), scale1)));
}

inline __m128 Vector3::get128() const
{
    return mVec128;
}

inline void storeXYZ(const Vector3 & vec, __m128 * quad)
{
    __m128 dstVec = *quad;
    VECTORMATH_ALIGNED(unsigned int sw[4]) = { 0, 0, 0, 0xFFFFFFFF };
    dstVec = sseSelect(vec.get128(), dstVec, sw);
    *quad = dstVec;
}

inline void loadXYZArray(Vector3 & vec0, Vector3 & vec1, Vector3 & vec2, Vector3 & vec3, const __m128 * threeQuads)
{
    const float * quads = (const float *)threeQuads;
    vec0 = Vector3(_mm_load_ps(quads));
    vec1 = Vector3(_mm_loadu_ps(quads + 3));
    vec2 = Vector3(_mm_loadu_ps(quads + 6));
    vec3 = Vector3(_mm_loadu_ps(quads + 9));
}

inline void storeXYZArray(const Vector3 & vec0, const Vector3 & vec1, const Vector3 & vec2, const Vector3 & vec3, __m128 * threeQuads)
{
    __m128 xxxx = _mm_shuffle_ps(vec1.get128(), vec1.get128(), _MM_SHUFFLE(0, 0, 0, 0));
    __m128 zzzz = _mm_shuffle_ps(vec2.get128(), vec2.get128(), _MM_SHUFFLE(2, 2, 2, 2));
    VECTORMATH_ALIGNED(unsigned int xsw[4]) = { 0, 0, 0, 0xFFFFFFFF };
    VECTORMATH_ALIGNED(unsigned int zsw[4]) = { 0xFFFFFFFF, 0, 0, 0 };
    threeQuads[0] = sseSelect(vec0.get128(), xxxx, xsw);
    threeQuads[1] = _mm_shuffle_ps(vec1.get128(), vec2.get128(), _MM_SHUFFLE(1, 0, 2, 1));
    threeQuads[2] = sseSelect(_mm_shuffle_ps(vec3.get128(), vec3.get128(), _MM_SHUFFLE(2, 1, 0, 3)), zzzz, zsw);
}

inline Vector3 & Vector3::setX(float _x)
{
    sseVecSetElement(mVec128, _x, 0);
    return *this;
}

inline Vector3 & Vector3::setX(const FloatInVec & _x)
{
    mVec128 = sseVecInsert(mVec128, _x.get128(), 0);
    return *this;
}

inline const FloatInVec Vector3::getX() const
{
    return FloatInVec(mVec128, 0);
}

inline Vector3 & Vector3::setY(float _y)
{
    sseVecSetElement(mVec128, _y, 1);
    return *this;
}

inline Vector3 & Vector3::setY(const FloatInVec & _y)
{
    mVec128 = sseVecInsert(mVec128, _y.get128(), 1);
    return *this;
}

inline const FloatInVec Vector3::getY() const
{
    return FloatInVec(mVec128, 1);
}

inline Vector3 & Vector3::setZ(float _z)
{
    sseVecSetElement(mVec128, _z, 2);
    return *this;
}

inline Vector3 & Vector3::setZ(const FloatInVec & _z)
{
    mVec128 = sseVecInsert(mVec128, _z.get128(), 2);
    return *this;
}

inline const FloatInVec Vector3::getZ() const
{
    return FloatInVec(mVec128, 2);
}

inline Vector3 & Vector3::setW(float _w)
{
    sseVecSetElement(mVec128, _w, 3);
    return *this;
}

inline Vector3 & Vector3::setW(const FloatInVec & _w)
{
    mVec128 = sseVecInsert(mVec128, _w.get128(), 3);
    return *this;
}

inline const FloatInVec Vector3::getW() const
{
    return FloatInVec(mVec128, 3);
}

inline Vector3 & Vector3::setElem(int idx, float value)
{
    sseVecSetElement(mVec128, value, idx);
    return *this;
}

inline Vector3 & Vector3::setElem(int idx, const FloatInVec & value)
{
    mVec128 = sseVecInsert(mVec128, value.get128(), idx);
    return *this;
}

inline const FloatInVec Vector3::getElem(int idx) const
{
    return FloatInVec(mVec128, idx);
}

inline VecIdx Vector3::operator[](int idx)
{
    return VecIdx(mVec128, idx);
}

inline const FloatInVec Vector3::operator[](int idx) const
{
    return FloatInVec(mVec128, idx);
}

inline const Vector3 Vector3::operator + (const Vector3 & vec) const
{
    return Vector3(_mm_add_ps(mVec128, vec.mVec128));
}

inline const Vector3 Vector3::operator - (const Vector3 & vec) const
{
    return Vector3(_mm_sub_ps(mVec128, vec.mVec128));
}

inline const Point3 Vector3::operator + (const Point3 & pnt) const
{
    return Point3(_mm_add_ps(mVec128, pnt.get128()));
}

inline const Vector3 Vector3::operator * (float scalar) const
{
    return *this * FloatInVec(scalar);
}

inline const Vector3 Vector3::operator * (const FloatInVec & scalar) const
{
    return Vector3(_mm_mul_ps(mVec128, scalar.get128()));
}

inline const Vector3 Vector3::operator * (const Vector3 & scalar) const
{
    return Vector3(_mm_mul_ps(mVec128, scalar.get128()));
}

inline Vector3 & Vector3::operator += (const Vector3 & vec)
{
    *this = *this + vec;
    return *this;
}

inline Vector3 & Vector3::operator -= (const Vector3 & vec)
{
    *this = *this - vec;
    return *this;
}

inline Vector3 & Vector3::operator *= (float scalar)
{
    *this = *this * scalar;
    return *this;
}

inline Vector3 & Vector3::operator *= (const FloatInVec & scalar)
{
    *this = *this * scalar;
    return *this;
}

inline const Vector3 Vector3::operator / (float scalar) const
{
    return *this / FloatInVec(scalar);
}

inline const Vector3 Vector3::operator / (const FloatInVec & scalar) const
{
    return Vector3(_mm_div_ps(mVec128, scalar.get128()));
}

inline Vector3 & Vector3::operator /= (float scalar)
{
    *this = *this / scalar;
    return *this;
}

inline Vector3 & Vector3::operator /= (const FloatInVec & scalar)
{
    *this = *this / scalar;
    return *this;
}

inline const Vector3 Vector3::operator - () const
{
    return Vector3(_mm_sub_ps(_mm_setzero_ps(), mVec128));
}


inline const BoolInVec Vector3::operator > (const Vector3 & vec) const
{
    auto output = FloatInVec(get128()) > FloatInVec(vec.get128());
    output.operator|=(BoolInVec(FloatInVec(0,0,0,1))); // Hide 4-th comparison result
    return output;
};

inline const BoolInVec Vector3::operator < (const Vector3 & vec) const
{
    auto output = FloatInVec(get128()) < FloatInVec(vec.get128());
    output.operator|=(BoolInVec(FloatInVec(0,0,0,1))); // Hide 4-th comparison result
    return output;
};

inline const Vector3 operator * (float scalar, const Vector3 & vec)
{
    return FloatInVec(scalar) * vec;
}

inline const Vector3 operator * (const FloatInVec & scalar, const Vector3 & vec)
{
    return vec * scalar;
}

inline const Vector3 mulPerElem(const Vector3 & vec0, const Vector3 & vec1)
{
    return Vector3(_mm_mul_ps(vec0.get128(), vec1.get128()));
}

inline const Vector3 divPerElem(const Vector3 & vec0, const Vector3 & vec1)
{
    return Vector3(_mm_div_ps(vec0.get128(), vec1.get128()));
}

inline const Vector3 recipPerElem(const Vector3 & vec)
{
    return Vector3(_mm_rcp_ps(vec.get128()));
}

inline const Vector3 absPerElem(const Vector3 & vec)
{
    return Vector3(sseFabsf(vec.get128()));
}

inline const Vector3 copySignPerElem(const Vector3 & vec0, const Vector3 & vec1)
{
    const __m128 vmask = sseUintToM128(0x7FFFFFFF);
    return Vector3(_mm_or_ps(
        _mm_and_ps(vmask, vec0.get128()),      // Value
        _mm_andnot_ps(vmask, vec1.get128()))); // Signs
}

inline const Vector3 maxPerElem(const Vector3 & vec0, const Vector3 & vec1)
{
    return Vector3(_mm_max_ps(vec0.get128(), vec1.get128()));
}

inline const FloatInVec maxElem(const Vector3 & vec)
{
    return FloatInVec(_mm_max_ps(_mm_max_ps(sseSplat(vec.get128(), 0), sseSplat(vec.get128(), 1)), sseSplat(vec.get128(), 2)));
}

inline const Vector3 minPerElem(const Vector3 & vec0, const Vector3 & vec1)
{
    return Vector3(_mm_min_ps(vec0.get128(), vec1.get128()));
}

inline const FloatInVec minElem(const Vector3 & vec)
{
    return FloatInVec(_mm_min_ps(_mm_min_ps(sseSplat(vec.get128(), 0), sseSplat(vec.get128(), 1)), sseSplat(vec.get128(), 2)));
}

inline const FloatInVec sum(const Vector3 & vec)
{
    return FloatInVec(_mm_add_ps(_mm_add_ps(sseSplat(vec.get128(), 0), sseSplat(vec.get128(), 1)), sseSplat(vec.get128(), 2)));
}

inline const FloatInVec dot(const Vector3 & vec0, const Vector3 & vec1)
{
    return FloatInVec(sseVecDot3(vec0.get128(), vec1.get128()), 0);
}

inline const FloatInVec lengthSqr(const Vector3 & vec)
{
    return FloatInVec(sseVecDot3(vec.get128(), vec.get128()), 0);
}

inline const FloatInVec length(const Vector3 & vec)
{
    return FloatInVec(_mm_sqrt_ps(sseVecDot3(vec.get128(), vec.get128())), 0);
}

inline const Vector3 normalizeApprox(const Vector3 & vec)
{
    return Vector3(_mm_mul_ps(vec.get128(), _mm_rsqrt_ps(sseVecDot3(vec.get128(), vec.get128()))));
}

inline const Vector3 normalize(const Vector3 & vec)
{
    return Vector3(_mm_mul_ps(vec.get128(), sseNewtonrapsonRSqrtf(sseVecDot3(vec.get128(), vec.get128()))));
}

inline const Vector3 cross(const Vector3 & vec0, const Vector3 & vec1)
{
    return Vector3(sseVecCross(vec0.get128(), vec1.get128()));
}

inline const Vector3 select(const Vector3 & vec0, const Vector3 & vec1, bool select1)
{
    return select(vec0, vec1, BoolInVec(select1));
}

inline const Vector3 select(const Vector3 & vec0, const Vector3 & vec1, const BoolInVec & select1)
{
    return Vector3(sseSelect(vec0.get128(), vec1.get128(), select1.get128()));
}

inline const Vector3 xorPerElem(const Vector3& a, const FloatInVec b)
{
    return Vector3(_mm_xor_ps(a.get128(), b.get128()));
}

inline const Vector3 sqrtPerElem(const Vector3 & vec)
{
    return Vector3(_mm_sqrt_ps(vec.get128()));
}

inline const Vector3 rSqrtEstNR(const Vector3& v)
{
    const __m128 nr = _mm_rsqrt_ps(v.get128());
    // Do one more Newton-Raphson step to improve precision.
    const __m128 muls = _mm_mul_ps(_mm_mul_ps(v.get128(), nr), nr);
    return Vector3(_mm_mul_ps(_mm_mul_ps(_mm_set_ps1(.5f), nr), _mm_sub_ps(_mm_set_ps1(3.f), muls)));
}

inline bool isNormalizedEst(const Vector3& v) {
	const __m128 max = _mm_set_ss(1.f + kNormalizationToleranceEstSq);
	const __m128 min = _mm_set_ss(1.f - kNormalizationToleranceEstSq);
	const __m128 dot = sseVecDot3(v.get128(), v.get128());
	const __m128 dotx000 = _mm_move_ss(_mm_setzero_ps(), dot);
	return (_mm_movemask_ps(
		_mm_and_ps(_mm_cmplt_ss(dotx000, max), _mm_cmpgt_ss(dotx000, min))) & 0x1) == 0x1;
}


inline bool Vector3::similar(const Vector3& other, float threshold) const
{
    return (lengthSqr((*this - other)) < FloatInVec(threshold));
}

#ifdef VECTORMATH_DEBUG

inline void print(const Vector3 & vec)
{
    SSEFloat tmp;
    tmp.m128 = vec.get128();
    std::printf("( %f %f %f )\n", tmp.f[0], tmp.f[1], tmp.f[2]);
}

inline void print(const Vector3 & vec, const char * name)
{
    SSEFloat tmp;
    tmp.m128 = vec.get128();
    std::printf("%s: ( %f %f %f )\n", name, tmp.f[0], tmp.f[1], tmp.f[2]);
}

#endif // VECTORMATH_DEBUG

// ========================================================
// Vector3d
// ========================================================

inline Vector3d::Vector3d(double _x, double _y, double _z)
{
	mVec256 = dsseSetr(_x, _y, _z, 0.0);
}

inline Vector3d::Vector3d(const DoubleInVec & _x, const DoubleInVec & _y, const DoubleInVec & _z)
{
	const DSSEVec4 xz = dsseMergeH(_x.get256(), _z.get256());
	mVec256 = dsseMergeH(xz, _y.get256());
}

inline Vector3d::Vector3d(const Point3 & pnt)
{
	mVec256 = dsseSetr((double)pnt.getX(), (double)pnt.getY(), (double)pnt.getZ(), (double)pnt.getW());
}

inline Vector3d::Vector3d(double scalar)
{
	mVec256 = dsseSet1(scalar);
}

inline Vector3d::Vector3d(const DoubleInVec & scalar)
{
	mVec256 = scalar.get256();
}

inline Vector3d::Vector3d(DSSEVec4 xyzw)
{
	mVec256 = xyzw;
}

inline const Vector3d Vector3d::xAxis()
{
	return Vector3d(dsseUnitVec1000());
}

inline const Vector3d Vector3d::yAxis()
{
	return Vector3d(dsseUnitVec0100());
}

inline const Vector3d Vector3d::zAxis()
{
	return Vector3d(dsseUnitVec0010());
}

inline const Vector3d lerp(double t, const Vector3d & vec0, const Vector3d & vec1)
{
	return lerp(DoubleInVec(t), vec0, vec1);
}

inline const Vector3d lerp(const DoubleInVec & t, const Vector3d & vec0, const Vector3d & vec1)
{
	return (vec0 + ((vec1 - vec0) * t));
}

inline const Vector3d slerp(double t, const Vector3d & unitVec0, const Vector3d & unitVec1)
{
	return slerp(DoubleInVec(t), unitVec0, unitVec1);
}

inline const Vector3d slerp(const DoubleInVec & t, const Vector3d & unitVec0, const Vector3d & unitVec1)
{
	DSSEVec4 scales, scale0, scale1, cosAngle, angle, tttt, oneMinusT, angles, sines;
	cosAngle = dsseVecDot3(unitVec0.get256(), unitVec1.get256());
	DSSEVec4 selectMask = dsseGt(dsseSet1(VECTORMATH_SLERP_TOL), cosAngle);
	angle = dsseACosf(cosAngle);
	tttt = t.get256();
	oneMinusT = dsseSub(dsseSet1(1.0), tttt);
	angles = dsseMergeH(dsseSet1(1.0), tttt); // angles = 1, t, 1, t
	angles = dsseMergeH(angles, oneMinusT);   // angles = 1, 1-t, t, 1-t
	angles = dsseMul(angles, angle);
	sines = dsseSinf(angles);
	scales = dsseDiv(sines, dsseSplat(sines, 0));
	scale0 = dsseSelect(oneMinusT, dsseSplat(scales, 1), selectMask);
	scale1 = dsseSelect(tttt, dsseSplat(scales, 2), selectMask);
	return Vector3d(dsseMAdd(unitVec0.get256(), scale0, dsseMul(unitVec1.get256(), scale1)));
}

inline DSSEVec4 Vector3d::get256() const
{
	return mVec256;
}

inline void storeXYZ(const Vector3d & vec, DSSEVec4 * quad)
{
	DSSEVec4 dstVec = *quad;
	VECTORMATH_ALIGNED(unsigned long long sw[4]) = { 0, 0, 0, 0xFFFFFFFFFFFFFFFF };
	dstVec = dsseSelect(vec.get256(), dstVec, sw);
	*quad = dstVec;
}

inline void loadXYZArray(Vector3d & vec0, Vector3d & vec1, Vector3d & vec2, Vector3d & vec3, const DSSEVec4 * threeQuads)
{
	const double * quads = (const double *)threeQuads;
	vec0 = Vector3d(dsseLoadu(quads + 0));
	vec1 = Vector3d(dsseLoadu(quads + 3));
	vec2 = Vector3d(dsseLoadu(quads + 6));
	vec3 = Vector3d(dsseLoadu(quads + 9));
}

inline void storeXYZArray(const Vector3d & vec0, const Vector3d & vec1, const Vector3d & vec2, const Vector3d & vec3, DSSEVec4 * threeQuads)
{
	DSSEVec4 xxxx = dsseShuffle(vec1.get256(), vec1.get256(), _MM_SHUFFLE(0, 0, 0, 0));
	DSSEVec4 zzzz = dsseShuffle(vec2.get256(), vec2.get256(), _MM_SHUFFLE(2, 2, 2, 2));
	VECTORMATH_ALIGNED(unsigned long long xsw[4]) = { 0, 0, 0, 0xFFFFFFFFFFFFFFFF };
	VECTORMATH_ALIGNED(unsigned long long zsw[4]) = { 0xFFFFFFFFFFFFFFFF, 0, 0, 0 };
	threeQuads[0] = dsseSelect(vec0.get256(), xxxx, xsw);
	threeQuads[1] = dsseShuffle(vec1.get256(), vec2.get256(), _MM_SHUFFLE(1, 0, 2, 1));
	threeQuads[2] = dsseSelect(dsseShuffle(vec3.get256(), vec3.get256(), _MM_SHUFFLE(2, 1, 0, 3)), zzzz, zsw);
}

inline Vector3d & Vector3d::operator = (const Vector3d & vec)
{
	mVec256 = vec.get256();
	return *this;
}

inline Vector3d & Vector3d::setX(double _x)
{
	dsseVecSetElement(mVec256, _x, 0);
	return *this;
}

inline Vector3d & Vector3d::setX(const DoubleInVec & _x)
{
	mVec256 = dsseVecInsert(mVec256, _x.get256(), 0);
	return *this;
}

inline const DoubleInVec Vector3d::getX() const
{
	return DoubleInVec(mVec256, 0);
}

inline Vector3d & Vector3d::setY(double _y)
{
	dsseVecSetElement(mVec256, _y, 1);
	return *this;
}

inline Vector3d & Vector3d::setY(const DoubleInVec & _y)
{
	mVec256 = dsseVecInsert(mVec256, _y.get256(), 1);
	return *this;
}

inline const DoubleInVec Vector3d::getY() const
{
	return DoubleInVec(mVec256, 1);
}

inline Vector3d & Vector3d::setZ(double _z)
{
	dsseVecSetElement(mVec256, _z, 2);
	return *this;
}

inline Vector3d & Vector3d::setZ(const DoubleInVec & _z)
{
	mVec256 = dsseVecInsert(mVec256, _z.get256(), 2);
	return *this;
}

inline const DoubleInVec Vector3d::getZ() const
{
	return DoubleInVec(mVec256, 2);
}

inline Vector3d & Vector3d::setW(double _w)
{
	dsseVecSetElement(mVec256, _w, 3);
	return *this;
}

inline Vector3d & Vector3d::setW(const DoubleInVec & _w)
{
	mVec256 = dsseVecInsert(mVec256, _w.get256(), 3);
	return *this;
}

inline const DoubleInVec Vector3d::getW() const
{
	return DoubleInVec(mVec256, 3);
}

inline Vector3d & Vector3d::setElem(int idx, double value)
{
	dsseVecSetElement(mVec256, value, idx);
	return *this;
}

inline Vector3d & Vector3d::setElem(int idx, const DoubleInVec & value)
{
	mVec256 = dsseVecInsert(mVec256, value.get256(), idx);
	return *this;
}

inline const DoubleInVec Vector3d::getElem(int idx) const
{
	return DoubleInVec(mVec256, idx);
}

inline VecIdxd Vector3d::operator[](int idx)
{
	return VecIdxd(mVec256, idx);
}

inline const DoubleInVec Vector3d::operator[](int idx) const
{
	return DoubleInVec(mVec256, idx);
}

inline const Vector3d Vector3d::operator + (const Vector3d & vec) const
{
	return Vector3d(dsseAdd(mVec256, vec.mVec256));
}

inline const Vector3d Vector3d::operator - (const Vector3d & vec) const
{
	return Vector3d(dsseSub(mVec256, vec.mVec256));
}

inline const Point3 Vector3d::operator + (const Point3 & pnt) const
{
	return Point3((float)mVec256.d[0] + pnt.getX(), (float)mVec256.d[1] + pnt.getY(), (float)mVec256.d[2] + pnt.getZ());
}

inline const Vector3d Vector3d::operator * (double scalar) const
{
	return *this * DoubleInVec(scalar);
}

inline const Vector3d Vector3d::operator * (const DoubleInVec & scalar) const
{
	return Vector3d(dsseMul(mVec256, scalar.get256()));
}

inline Vector3d & Vector3d::operator += (const Vector3d & vec)
{
	*this = *this + vec;
	return *this;
}

inline Vector3d & Vector3d::operator -= (const Vector3d & vec)
{
	*this = *this - vec;
	return *this;
}

inline Vector3d & Vector3d::operator *= (double scalar)
{
	*this = *this * scalar;
	return *this;
}

inline Vector3d & Vector3d::operator *= (const DoubleInVec & scalar)
{
	*this = *this * scalar;
	return *this;
}

inline const Vector3d Vector3d::operator / (double scalar) const
{
	return *this / DoubleInVec(scalar);
}

inline const Vector3d Vector3d::operator / (const DoubleInVec & scalar) const
{
	return Vector3d(dsseDiv(mVec256, scalar.get256()));
}

inline Vector3d & Vector3d::operator /= (double scalar)
{
	*this = *this / scalar;
	return *this;
}

inline Vector3d & Vector3d::operator /= (const DoubleInVec & scalar)
{
	*this = *this / scalar;
	return *this;
}

inline const Vector3d Vector3d::operator - () const
{
	return Vector3d(dsseNegatef(mVec256));
}

inline const Vector3d operator * (double scalar, const Vector3d & vec)
{
	return DoubleInVec(scalar) * vec;
}

inline const Vector3d operator * (const DoubleInVec & scalar, const Vector3d & vec)
{
	return vec * scalar;
}

inline const Vector3d mulPerElem(const Vector3d & vec0, const Vector3d & vec1)
{
	return Vector3d(dsseMul(vec0.get256(), vec1.get256()));
}

inline const Vector3d divPerElem(const Vector3d & vec0, const Vector3d & vec1)
{
	return Vector3d(dsseDiv(vec0.get256(), vec1.get256()));
}

inline const Vector3d recipPerElem(const Vector3d & vec)
{
	return Vector3d(dsseRecipf(vec.get256()));
}

inline const Vector3d absPerElem(const Vector3d & vec)
{
	return Vector3d(dsseFabsf(vec.get256()));
}

inline const Vector3d copySignPerElem(const Vector3d & vec0, const Vector3d & vec1)
{
	return Vector3d(dsseCopySign(vec0.get256(), vec1.get256()));
}

inline const Vector3d maxPerElem(const Vector3d & vec0, const Vector3d & vec1)
{
	return Vector3d(dsseMax(vec0.get256(), vec1.get256()));
}

inline const DoubleInVec maxElem(const Vector3d & vec)
{
	return DoubleInVec(dsseMax(dsseMax(dsseSplat(vec.get256(), 0), dsseSplat(vec.get256(), 1)), dsseSplat(vec.get256(), 2)));
}

inline const Vector3d minPerElem(const Vector3d & vec0, const Vector3d & vec1)
{
	return Vector3d(dsseMin(vec0.get256(), vec1.get256()));
}

inline const DoubleInVec minElem(const Vector3d & vec)
{
	return DoubleInVec(dsseMin(dsseMin(dsseSplat(vec.get256(), 0), dsseSplat(vec.get256(), 1)), dsseSplat(vec.get256(), 2)));
}

inline const DoubleInVec sum(const Vector3d & vec)
{
	return DoubleInVec(dsseAdd(dsseAdd(dsseSplat(vec.get256(), 0), dsseSplat(vec.get256(), 1)), dsseSplat(vec.get256(), 2)));
}

inline const DoubleInVec dot(const Vector3d & vec0, const Vector3d & vec1)
{
	return DoubleInVec(dsseVecDot3(vec0.get256(), vec1.get256()), 0);
}

inline const DoubleInVec lengthSqr(const Vector3d & vec)
{
	return DoubleInVec(dsseVecDot3(vec.get256(), vec.get256()), 0);
}

inline const DoubleInVec length(const Vector3d & vec)
{
	return DoubleInVec(dsseSqrtf(dsseVecDot3(vec.get256(), vec.get256())), 0);
}

inline const Vector3d normalizeApprox(const Vector3d & vec)
{
	return Vector3d(dsseMul(vec.get256(), dsseRSqrtf(dsseVecDot3(vec.get256(), vec.get256()))));
}

inline const Vector3d normalize(const Vector3d & vec)
{
	return Vector3d(dsseMul(vec.get256(), dsseNewtonrapsonRSqrtf(dsseVecDot3(vec.get256(), vec.get256()))));
}

inline const Vector3d cross(const Vector3d & vec0, const Vector3d & vec1)
{
	return Vector3d(dsseVecCross(vec0.get256(), vec1.get256()));
}

inline const Vector3d select(const Vector3d & vec0, const Vector3d & vec1, bool select1)
{
	return select(vec0, vec1, BoolInVec(select1));
}

inline const Vector3d select(const Vector3d & vec0, const Vector3d & vec1, const BoolInVec & select1)
{
	return Vector3d(dsseSelect(vec0.get256(), vec1.get256(), dsseFromBool(select1)));
}

inline const Vector3d xorPerElem(const Vector3d& a, const DoubleInVec b)
{
	return Vector3d(dsseXor(a.get256(), b.get256()));
}

inline const Vector3d sqrtPerElem(const Vector3d & vec)
{
	return Vector3d(dsseSqrtf(vec.get256()));
}

inline const Vector3d rSqrtEstNR(const Vector3d& v)
{
	return Vector3d(dsseNewtonrapsonRSqrtf(v.get256()));
}

inline bool isNormalizedEst(const Vector3d& v) {
	const __m128d max = _mm_set_sd(1.0 + kdNormalizationToleranceEstSq);
	const __m128d min = _mm_set_sd(1.0 - kdNormalizationToleranceEstSq);
	const __m128d dot = dsseVecDot3(v.get256(), v.get256()).xy;
	const __m128d dotx000 = _mm_move_sd(_mm_setzero_pd(), dot);
	return (_mm_movemask_pd(
		_mm_and_pd(_mm_cmplt_sd(dotx000, max), _mm_cmpgt_sd(dotx000, min))) & 0x1) == 0x1;
}

inline const DSSEVec4 cmpEq(const Vector3d& a, const Vector3d& b) {
    return dsseEq(a.get256(), b.get256());
}

inline const DSSEVec4 cmpNotEq(const Vector3d& a, const Vector3d& b) {
    return dsseNe(a.get256(), b.get256());
}

inline const DSSEVec4 cmpLt(const Vector3d& a, const Vector3d& b) {
    return dsseLt(a.get256(), b.get256());
}

inline const DSSEVec4 cmpLe(const Vector3d& a, const Vector3d& b) {
    return dsseLe(a.get256(), b.get256());
}

inline const DSSEVec4 cmpGt(const Vector3d& a, const Vector3d& b) {
    return dsseGt(a.get256(), b.get256());
}

inline const DSSEVec4 cmpGe(const Vector3d& a, const Vector3d& b) {
    return dsseGe(a.get256(), b.get256());
}


#ifdef VECTORMATH_DEBUG

inline void print(const Vector3d & vec)
{
	std::printf("( %f %f %f )\n", vec.get256().d[0], vec.get256().d[1], vec.get256().d[2]);
}

inline void print(const Vector3d & vec, const char * name)
{
	std::printf("%s: ( %f %f %f )\n", name, vec.get256().d[0], vec.get256().d[1], vec.get256().d[2]);
}

#endif // VECTORMATH_DEBUG

// ========================================================
// Vector4
// ========================================================

inline Vector4::Vector4(float _x, float _y, float _z, float _w)
{
    mVec128 = _mm_setr_ps(_x, _y, _z, _w);
}

inline Vector4::Vector4(const FloatInVec & _x, const FloatInVec & _y, const FloatInVec & _z, const FloatInVec & _w)
{
    mVec128 = _mm_unpacklo_ps(
        _mm_unpacklo_ps(_x.get128(), _z.get128()),
        _mm_unpacklo_ps(_y.get128(), _w.get128()));
}

inline Vector4::Vector4(const Vector3 & xyz, float _w)
{
    mVec128 = xyz.get128();
    sseVecSetElement(mVec128, _w, 3);
}

inline Vector4::Vector4(const Vector3 & xyz, const FloatInVec & _w)
{
    mVec128 = xyz.get128();
    mVec128 = sseVecInsert(mVec128, _w.get128(), 3);
}

inline Vector4::Vector4(const Vector3 & vec)
{
    mVec128 = vec.get128();
    mVec128 = sseVecInsert(mVec128, _mm_setzero_ps(), 3);
}

inline Vector4::Vector4(const Point3 & pnt)
{
    mVec128 = pnt.get128();
    mVec128 = sseVecInsert(mVec128, _mm_set1_ps(1.0f), 3);
}

inline Vector4::Vector4(const Quat & quat)
{
    mVec128 = quat.get128();
}

inline Vector4::Vector4(float scalar)
{
    mVec128 = FloatInVec(scalar).get128();
}

inline Vector4::Vector4(const FloatInVec & scalar)
{
    mVec128 = scalar.get128();
}

inline Vector4::Vector4(__m128 vf4)
{
    mVec128 = vf4;
}

//========================================= #TheForgeMathExtensionsBegin ================================================
//========================================= #TheForgeAnimationMathExtensionsBegin =======================================

// Construct zero vector
//
inline const Vector3 Vector3::zero()
{
	return Vector3(_mm_setr_ps(0.0f, 0.0f, 0.0f, 0.0f));
}

// Construct one vector
//
inline const Vector3 Vector3::one()
{
	return Vector3(_mm_setr_ps(1.0f, 1.0f, 1.0f, 0.0f));
}

//========================================= #TheForgeAnimationMathExtensionsEnd =======================================
//========================================= #TheForgeMathExtensionsEnd ================================================
// 
//========================================= #TheForgeMathExtensionsBegin ================================================
//========================================= #TheForgeAnimationMathExtensionsBegin =======================================

inline const Vector4 Vector4::fromVector4Int(const Vector4Int vecInt)
{
	Vector4 ret = {};
    ret.mVec128 = _mm_cvtepi32_ps(vecInt);
	return ret;
}

//========================================= #TheForgeAnimationMathExtensionsEnd =======================================
//========================================= #TheForgeMathExtensionsEnd ================================================

inline const Vector4 Vector4::xAxis()
{
    return Vector4(sseUnitVec1000());
}

inline const Vector4 Vector4::yAxis()
{
    return Vector4(sseUnitVec0100());
}

inline const Vector4 Vector4::zAxis()
{
    return Vector4(sseUnitVec0010());
}

inline const Vector4 Vector4::wAxis()
{
    return Vector4(sseUnitVec0001());
}

//========================================= #TheForgeMathExtensionsBegin ================================================
//========================================= #TheForgeAnimationMathExtensionsBegin =======================================

inline const Vector4 Vector4::zero()
{
    return Vector4(_mm_setr_ps(0.0f, 0.0f, 0.0f, 0.0f));
}

inline const Vector4 Vector4::one()
{
    return Vector4(_mm_setr_ps(1.0f, 1.0f, 1.0f, 1.0f));
}

//========================================= #TheForgeAnimationMathExtensionsEnd =======================================
//========================================= #TheForgeMathExtensionsEnd ================================================

inline bool Vector4::similar(const Vector4& other, float threshold) const
{
    return (lengthSqr((*this - other)) < FloatInVec(threshold));
}

inline const Vector4 lerp(float t, const Vector4 & vec0, const Vector4 & vec1)
{
    return lerp(FloatInVec(t), vec0, vec1);
}

inline const Vector4 lerp(const FloatInVec & t, const Vector4 & vec0, const Vector4 & vec1)
{
    return (vec0 + ((vec1 - vec0) * t));
}

inline const Vector4 slerp(float t, const Vector4 & unitVec0, const Vector4 & unitVec1)
{
    return slerp(FloatInVec(t), unitVec0, unitVec1);
}

inline const Vector4 slerp(const FloatInVec & t, const Vector4 & unitVec0, const Vector4 & unitVec1)
{
    __m128 scales, scale0, scale1, cosAngle, angle, tttt, oneMinusT, angles, sines;
    cosAngle = sseVecDot4(unitVec0.get128(), unitVec1.get128());
    __m128 selectMask = _mm_cmpgt_ps(_mm_set1_ps(VECTORMATH_SLERP_TOL), cosAngle);
    angle = sseACosf(cosAngle);
    tttt = t.get128();
    oneMinusT = _mm_sub_ps(_mm_set1_ps(1.0f), tttt);
    angles = _mm_unpacklo_ps(_mm_set1_ps(1.0f), tttt); // angles = 1, t, 1, t
    angles = _mm_unpacklo_ps(angles, oneMinusT);       // angles = 1, 1-t, t, 1-t
    angles = _mm_mul_ps(angles, angle);
    sines = sseSinf(angles);
    scales = _mm_div_ps(sines, sseSplat(sines, 0));
    scale0 = sseSelect(oneMinusT, sseSplat(scales, 1), selectMask);
    scale1 = sseSelect(tttt, sseSplat(scales, 2), selectMask);
    return Vector4(sseMAdd(unitVec0.get128(), scale0, _mm_mul_ps(unitVec1.get128(), scale1)));
}

inline __m128& Vector4::get128()
{
    return mVec128;
}

inline __m128 Vector4::get128() const
{
    return mVec128;
}

inline Vector4 & Vector4::setXYZ(const Vector3 & vec)
{
    VECTORMATH_ALIGNED(unsigned int sw[4]) = { 0, 0, 0, 0xFFFFFFFF };
    mVec128 = sseSelect(vec.get128(), mVec128, sw);
    return *this;
}

inline const Vector3 Vector4::getXYZ() const
{
    return Vector3(mVec128);
}

inline Vector4 & Vector4::setX(float _x)
{
    sseVecSetElement(mVec128, _x, 0);
    return *this;
}

inline Vector4 & Vector4::setX(const FloatInVec & _x)
{
    mVec128 = sseVecInsert(mVec128, _x.get128(), 0);
    return *this;
}

inline const FloatInVec Vector4::getX() const
{
    return FloatInVec(mVec128, 0);
}

inline Vector4 & Vector4::setY(float _y)
{
    sseVecSetElement(mVec128, _y, 1);
    return *this;
}

inline Vector4 & Vector4::setY(const FloatInVec & _y)
{
    mVec128 = sseVecInsert(mVec128, _y.get128(), 1);
    return *this;
}

inline const FloatInVec Vector4::getY() const
{
    return FloatInVec(mVec128, 1);
}

inline Vector4 & Vector4::setZ(float _z)
{
    sseVecSetElement(mVec128, _z, 2);
    return *this;
}

inline Vector4 & Vector4::setZ(const FloatInVec & _z)
{
    mVec128 = sseVecInsert(mVec128, _z.get128(), 2);
    return *this;
}

inline const FloatInVec Vector4::getZ() const
{
    return FloatInVec(mVec128, 2);
}

inline Vector4 & Vector4::setW(float _w)
{
    sseVecSetElement(mVec128, _w, 3);
    return *this;
}

inline Vector4 & Vector4::setW(const FloatInVec & _w)
{
    mVec128 = sseVecInsert(mVec128, _w.get128(), 3);
    return *this;
}

inline const FloatInVec Vector4::getW() const
{
    return FloatInVec(mVec128, 3);
}

inline Vector4 & Vector4::setElem(int idx, float value)
{
    sseVecSetElement(mVec128, value, idx);
    return *this;
}

inline Vector4 & Vector4::setElem(int idx, const FloatInVec & value)
{
    mVec128 = sseVecInsert(mVec128, value.get128(), idx);
    return *this;
}

inline const FloatInVec Vector4::getElem(int idx) const
{
    return FloatInVec(mVec128, idx);
}

inline VecIdx Vector4::operator[](int idx)
{
    return VecIdx(mVec128, idx);
}

inline const FloatInVec Vector4::operator[](int idx) const
{
    return FloatInVec(mVec128, idx);
}

inline const Vector4 Vector4::operator + (const Vector4 & vec) const
{
    return Vector4(_mm_add_ps(mVec128, vec.mVec128));
}

inline const Vector4 Vector4::operator - (const Vector4 & vec) const
{
    return Vector4(_mm_sub_ps(mVec128, vec.mVec128));
}

inline const Vector4 Vector4::operator * (float scalar) const
{
    return *this * FloatInVec(scalar);
}

inline const Vector4 Vector4::operator * (const FloatInVec & scalar) const
{
    return Vector4(_mm_mul_ps(mVec128, scalar.get128()));
}

inline const Vector4 Vector4::operator * (const Vector4 & scalar) const
{
    return Vector4(_mm_mul_ps(mVec128, scalar.get128()));
}

inline Vector4 & Vector4::operator += (const Vector4 & vec)
{
    *this = *this + vec;
    return *this;
}

inline Vector4 & Vector4::operator -= (const Vector4 & vec)
{
    *this = *this - vec;
    return *this;
}

inline Vector4 & Vector4::operator *= (float scalar)
{
    *this = *this * scalar;
    return *this;
}

inline Vector4 & Vector4::operator *= (const FloatInVec & scalar)
{
    *this = *this * scalar;
    return *this;
}

inline const Vector4 Vector4::operator / (float scalar) const
{
    return *this / FloatInVec(scalar);
}

inline const Vector4 Vector4::operator / (const FloatInVec & scalar) const
{
    return Vector4(_mm_div_ps(mVec128, scalar.get128()));
}

inline Vector4 & Vector4::operator /= (float scalar)
{
    *this = *this / scalar;
    return *this;
}

inline Vector4 & Vector4::operator /= (const FloatInVec & scalar)
{
    *this = *this / scalar;
    return *this;
}

inline const Vector4 Vector4::operator - () const
{
    return Vector4(_mm_sub_ps(_mm_setzero_ps(), mVec128));
}

inline const Vector4 operator * (float scalar, const Vector4 & vec)
{
    return FloatInVec(scalar) * vec;
}

inline const Vector4 operator * (const FloatInVec & scalar, const Vector4 & vec)
{
    return vec * scalar;
}

inline const Vector4 mulPerElem(const Vector4 & vec0, const Vector4 & vec1)
{
    return Vector4(_mm_mul_ps(vec0.get128(), vec1.get128()));
}

inline const Vector4 divPerElem(const Vector4 & vec0, const Vector4 & vec1)
{
    return Vector4(_mm_div_ps(vec0.get128(), vec1.get128()));
}

inline const Vector4 recipPerElem(const Vector4 & vec)
{
    return Vector4(_mm_rcp_ps(vec.get128()));
}

//========================================= #TheForgeMathExtensionsBegin ================================================
//========================================= #TheForgeAnimationMathExtensionsBegin =======================================

inline const Vector4 sqrtPerElem(const Vector4 & vec)
{
    return Vector4(_mm_sqrt_ps(vec.get128()));
}

inline const Vector4 rsqrtPerElem(const Vector4 & vec)
{
    return Vector4(_mm_rsqrt_ps(vec.get128()));
}

inline const Vector4 rcpEst(const Vector4& v) {
    return Vector4(_mm_rcp_ps(v.get128()));
}

inline const Vector4 rSqrtEst(const Vector4& v) {
  return Vector4(_mm_rsqrt_ps(v.get128()));
}

inline const Vector4 rSqrtEstNR(const Vector4& v) {
  const __m128 nr = _mm_rsqrt_ps(v.get128());
  // Do one more Newton-Raphson step to improve precision.
  const __m128 muls = _mm_mul_ps(_mm_mul_ps(v.get128(), nr), nr);
  return Vector4(_mm_mul_ps(_mm_mul_ps(_mm_set_ps1(.5f), nr), _mm_sub_ps(_mm_set_ps1(3.f), muls)));
}

inline const Vector4 aCos(const Vector4& arg)
{
    return Vector4(sseACosf(arg.get128()));
}

//========================================= #TheForgeAnimationMathExtensionsEnd =======================================
//========================================= #TheForgeMathExtensionsEnd ================================================

inline const Vector4 absPerElem(const Vector4 & vec)
{
    return Vector4(sseFabsf(vec.get128()));
}

inline const Vector4 copySignPerElem(const Vector4 & vec0, const Vector4 & vec1)
{
    const __m128 vmask = sseUintToM128(0x7FFFFFFF);
    return Vector4(_mm_or_ps(
        _mm_and_ps(vmask, vec0.get128()),      // Value
        _mm_andnot_ps(vmask, vec1.get128()))); // Signs
}

inline const Vector4 maxPerElem(const Vector4 & vec0, const Vector4 & vec1)
{
    return Vector4(_mm_max_ps(vec0.get128(), vec1.get128()));
}

inline const FloatInVec maxElem(const Vector4 & vec)
{
    return FloatInVec(_mm_max_ps(
        _mm_max_ps(sseSplat(vec.get128(), 0), sseSplat(vec.get128(), 1)),
        _mm_max_ps(sseSplat(vec.get128(), 2), sseSplat(vec.get128(), 3))));
}

inline const Vector4 minPerElem(const Vector4 & vec0, const Vector4 & vec1)
{
    return Vector4(_mm_min_ps(vec0.get128(), vec1.get128()));
}

inline const FloatInVec minElem(const Vector4 & vec)
{
    return FloatInVec(_mm_min_ps(
        _mm_min_ps(sseSplat(vec.get128(), 0), sseSplat(vec.get128(), 1)),
        _mm_min_ps(sseSplat(vec.get128(), 2), sseSplat(vec.get128(), 3))));
}

inline const FloatInVec ceil(const Vector4 & vec)
{
    return FloatInVec(_mm_round_ps(vec.get128(), _MM_FROUND_TO_POS_INF|_MM_FROUND_NO_EXC));
}

inline const FloatInVec sum(const Vector4 & vec)
{
    return FloatInVec(_mm_add_ps(
        _mm_add_ps(sseSplat(vec.get128(), 0), sseSplat(vec.get128(), 1)),
        _mm_add_ps(sseSplat(vec.get128(), 2), sseSplat(vec.get128(), 3))));
}

inline const FloatInVec dot(const Vector4 & vec0, const Vector4 & vec1)
{
    return FloatInVec(sseVecDot4(vec0.get128(), vec1.get128()), 0);
}

inline const FloatInVec lengthSqr(const Vector4 & vec)
{
    return FloatInVec(sseVecDot4(vec.get128(), vec.get128()), 0);
}

inline const FloatInVec length(const Vector4 & vec)
{
    return FloatInVec(_mm_sqrt_ps(sseVecDot4(vec.get128(), vec.get128())), 0);
}

inline const Vector4 normalizeApprox(const Vector4 & vec)
{
    return Vector4(_mm_mul_ps(vec.get128(), _mm_rsqrt_ps(sseVecDot4(vec.get128(), vec.get128()))));
}

inline const Vector4 normalize(const Vector4 & vec)
{
    return Vector4(_mm_mul_ps(vec.get128(), sseNewtonrapsonRSqrtf(sseVecDot4(vec.get128(), vec.get128()))));
}

inline const Vector4 select(const Vector4 & vec0, const Vector4 & vec1, bool select1)
{
    return select(vec0, vec1, BoolInVec(select1));
}

inline const Vector4 select(const Vector4 & vec0, const Vector4 & vec1, const BoolInVec & select1)
{
    return Vector4(sseSelect(vec0.get128(), vec1.get128(), select1.get128()));
}

//========================================= #TheForgeMathExtensionsBegin ================================================
//========================================= #TheForgeAnimationMathExtensionsBegin =======================================

inline const Vector4Int cmpEq(const Vector4& a, const Vector4& b) {
    return _mm_castps_si128(_mm_cmpeq_ps(a.get128(), b.get128()));
}

inline const Vector4Int cmpNotEq(const Vector4& a, const Vector4& b) {
    return _mm_castps_si128(_mm_cmpneq_ps(a.get128(), b.get128()));
}

inline const Vector4Int cmpLt(const Vector4& a, const Vector4& b) {
    return _mm_castps_si128(_mm_cmplt_ps(a.get128(), b.get128()));
}

inline const Vector4Int cmpLe(const Vector4& a, const Vector4& b) {
    return _mm_castps_si128(_mm_cmple_ps(a.get128(), b.get128()));
}

inline const Vector4Int cmpGt(const Vector4& a, const Vector4& b) {
    return _mm_castps_si128(_mm_cmpgt_ps(a.get128(), b.get128()));
}

inline const Vector4Int cmpGe(const Vector4& a, const Vector4& b) {
    return _mm_castps_si128(_mm_cmpge_ps(a.get128(), b.get128()));
}

inline const Vector4Int signBit(const Vector4& v) {
    return _mm_slli_epi32(_mm_srli_epi32(_mm_castps_si128(v.get128()), 31), 31);
}

inline const Vector4 xorPerElem(const Vector4& a, const Vector4Int b) {
    return Vector4(_mm_xor_ps(a.get128(), _mm_castsi128_ps(b)));
}

inline const Vector4 orPerElem(const Vector4& a, const Vector4Int b) {
    return Vector4(_mm_or_ps(a.get128(), _mm_castsi128_ps(b)));
}

inline const Vector4 orPerElem(const Vector4& a, const Vector4& b) {
    return Vector4(_mm_or_ps(a.get128(), b.get128()));
}

inline const Vector4 andPerElem(const Vector4& a, const Vector4Int b) {
    return Vector4(_mm_and_ps(a.get128(), _mm_castsi128_ps(b)));
}

inline const Vector4 andPerElem(const Vector4& a, const Vector4& b) {
    return Vector4(_mm_and_ps(a.get128(), b.get128()));
}

inline const Vector4 halfToFloat(const Vector4Int vecInt) {
    const __m128i mask_nosign = _mm_set1_epi32(0x7fff);
    const __m128 magic = _mm_castsi128_ps(_mm_set1_epi32((254 - 15) << 23));
    const __m128i was_infnan = _mm_set1_epi32(0x7bff);
    const __m128 exp_infnan = _mm_castsi128_ps(_mm_set1_epi32(255 << 23));

    const __m128i expmant = _mm_and_si128(mask_nosign, vecInt);
    const __m128i shifted = _mm_slli_epi32(expmant, 13);
    const __m128 scaled = _mm_mul_ps(_mm_castsi128_ps(shifted), magic);
    const __m128i b_wasinfnan = _mm_cmpgt_epi32(expmant, was_infnan);
    const __m128i sign = _mm_slli_epi32(_mm_xor_si128(vecInt, expmant), 16);
    const __m128 infnanexp =
        _mm_and_ps(_mm_castsi128_ps(b_wasinfnan), exp_infnan);
    const __m128 sign_inf = _mm_or_ps(_mm_castsi128_ps(sign), infnanexp);
    return Vector4(_mm_or_ps(scaled, sign_inf));
}

inline void transpose3x4(const Vector4 in[3], Vector4 out[4]) {
    const __m128 zero = _mm_setzero_ps();
    const __m128 temp0 = _mm_unpacklo_ps(in[0].get128(), in[1].get128());
    const __m128 temp1 = _mm_unpacklo_ps(in[2].get128(), zero);
    const __m128 temp2 = _mm_unpackhi_ps(in[0].get128(), in[1].get128());
    const __m128 temp3 = _mm_unpackhi_ps(in[2].get128(), zero);
    out[0] = Vector4(_mm_movelh_ps(temp0, temp1));
    out[1] = Vector4(_mm_movehl_ps(temp1, temp0));
    out[2] = Vector4(_mm_movelh_ps(temp2, temp3));
    out[3] = Vector4(_mm_movehl_ps(temp3, temp2));
}

inline void transpose4x4(const Vector4 in[4], Vector4 out[4]) {
    const __m128 tmp0 = _mm_unpacklo_ps(in[0].get128(), in[2].get128());
    const __m128 tmp1 = _mm_unpacklo_ps(in[1].get128(), in[3].get128());
    const __m128 tmp2 = _mm_unpackhi_ps(in[0].get128(), in[2].get128());
    const __m128 tmp3 = _mm_unpackhi_ps(in[1].get128(), in[3].get128());
    out[0] = Vector4(_mm_unpacklo_ps(tmp0, tmp1));
    out[1] = Vector4(_mm_unpackhi_ps(tmp0, tmp1));
    out[2] = Vector4(_mm_unpacklo_ps(tmp2, tmp3));
    out[3] = Vector4(_mm_unpackhi_ps(tmp2, tmp3));
}

//CONFFX_TEST_BEGIN
inline void transpose4x3(const Vector4 in[4], Vector4 out[4]) {
	const __m128 tmp0 = _mm_unpacklo_ps(in[0].get128(), in[2].get128());
	const __m128 tmp1 = _mm_unpacklo_ps(in[1].get128(), in[3].get128());
	const __m128 tmp2 = _mm_unpackhi_ps(in[0].get128(), in[2].get128());
	const __m128 tmp3 = _mm_unpackhi_ps(in[1].get128(), in[3].get128());
	out[0] = Vector4(_mm_unpacklo_ps(tmp0, tmp1));
	out[1] = Vector4(_mm_unpackhi_ps(tmp0, tmp1));
	out[2] = Vector4(_mm_unpacklo_ps(tmp2, tmp3));
}
//CONFFX__TEST_END

inline void transpose16x16(const Vector4 in[16], Vector4 out[16]) {
    const __m128 tmp0 = _mm_unpacklo_ps(in[0].get128(), in[2].get128());
    const __m128 tmp1 = _mm_unpacklo_ps(in[1].get128()  , in[3].get128());
    const __m128 tmp2 = _mm_unpackhi_ps(in[0].get128()  , in[2].get128());
    const __m128 tmp3 = _mm_unpackhi_ps(in[1].get128()  , in[3].get128());
    const __m128 tmp4 = _mm_unpacklo_ps(in[4].get128()  , in[6].get128());
    const __m128 tmp5 = _mm_unpacklo_ps(in[5].get128()  , in[7].get128());
    const __m128 tmp6 = _mm_unpackhi_ps(in[4].get128()  , in[6].get128());
    const __m128 tmp7 = _mm_unpackhi_ps(in[5].get128()  , in[7].get128());
    const __m128 tmp8 = _mm_unpacklo_ps(in[8].get128()  , in[10].get128());
    const __m128 tmp9 = _mm_unpacklo_ps(in[9].get128()  , in[11].get128());
    const __m128 tmp10 = _mm_unpackhi_ps(in[8].get128(), in[10].get128());
    const __m128 tmp11 = _mm_unpackhi_ps(in[9].get128(), in[11].get128());
    const __m128 tmp12 = _mm_unpacklo_ps(in[12].get128(), in[14].get128());
    const __m128 tmp13 = _mm_unpacklo_ps(in[13].get128(), in[15].get128());
    const __m128 tmp14 = _mm_unpackhi_ps(in[12].get128(), in[14].get128());
    const __m128 tmp15 = _mm_unpackhi_ps(in[13].get128(), in[15].get128());
    out[0] = Vector4(_mm_unpacklo_ps(tmp0, tmp1));
    out[1] = Vector4(_mm_unpacklo_ps(tmp4, tmp5));
    out[2] = Vector4(_mm_unpacklo_ps(tmp8, tmp9));
    out[3] = Vector4(_mm_unpacklo_ps(tmp12, tmp13));
    out[4] = Vector4(_mm_unpackhi_ps(tmp0, tmp1));
    out[5] = Vector4(_mm_unpackhi_ps(tmp4, tmp5));
    out[6] = Vector4(_mm_unpackhi_ps(tmp8, tmp9));
    out[7] = Vector4(_mm_unpackhi_ps(tmp12, tmp13));
    out[8] = Vector4(_mm_unpacklo_ps(tmp2, tmp3));
    out[9] = Vector4(_mm_unpacklo_ps(tmp6, tmp7));
    out[10] = Vector4(_mm_unpacklo_ps(tmp10, tmp11));
    out[11] = Vector4(_mm_unpacklo_ps(tmp14, tmp15));
    out[12] = Vector4(_mm_unpackhi_ps(tmp2, tmp3));
    out[13] = Vector4(_mm_unpackhi_ps(tmp6, tmp7));
    out[14] = Vector4(_mm_unpackhi_ps(tmp10, tmp11));
    out[15] = Vector4(_mm_unpackhi_ps(tmp14, tmp15));
}

inline void storePtrU(const Vector4& v, float* f) {
    _mm_storeu_ps(f, v.get128());
}

inline void store3PtrU(const Vector4& v, float* f) {
    _mm_store_ss(f + 0, v.get128());
    const __m128 a = _mm_shuffle_ps(v.get128(), v.get128(), _MM_SHUFFLE(1, 1, 1, 1));
    _mm_store_ss(f + 1, a);
    _mm_store_ss(f + 2, _mm_movehl_ps(v.get128(), v.get128()));
}

//========================================= #TheForgeAnimationMathExtensionsEnd =======================================
//========================================= #TheForgeMathExtensionsEnd ================================================

#ifdef VECTORMATH_DEBUG

inline void print(const Vector4 & vec)
{
    SSEFloat tmp;
    tmp.m128 = vec.get128();
    std::printf("( %f %f %f %f )\n", tmp.f[0], tmp.f[1], tmp.f[2], tmp.f[3]);
}

inline void print(const Vector4 & vec, const char * name)
{
    SSEFloat tmp;
    tmp.m128 = vec.get128();
    std::printf("%s: ( %f %f %f %f )\n", name, tmp.f[0], tmp.f[1], tmp.f[2], tmp.f[3]);
}

#endif // VECTORMATH_DEBUG

// ========================================================
// Vector4d
// ========================================================

inline Vector4d::Vector4d(double _x, double _y, double _z, double _w)
{
	mVec256 = dsseSetr(_x, _y, _z, _w);
}

inline Vector4d::Vector4d(const DoubleInVec & _x, const DoubleInVec & _y, const DoubleInVec & _z, const DoubleInVec & _w)
{
	mVec256 = dsseMergeH(
		dsseMergeH(_x.get256(), _z.get256()),
		dsseMergeH(_y.get256(), _w.get256())
	);
}

inline Vector4d::Vector4d(const Vector3d & xyz, double _w)
{
	mVec256 = xyz.get256();
	dsseVecSetElement(mVec256, _w, 3);
}

inline Vector4d::Vector4d(const Vector3d & xyz, const DoubleInVec & _w)
{
	mVec256 = xyz.get256();
	mVec256 = dsseVecInsert(mVec256, _w.get256(), 3);
}

inline Vector4d::Vector4d(const Vector3d & vec)
{
	mVec256 = vec.get256();
	dsseVecSetElement(mVec256, 0.0, 3);
}

inline Vector4d::Vector4d(const Point3 & pnt)
{
	mVec256 = dsseSetr((double)pnt.getX(), (double)pnt.getY(), (double)pnt.getZ(), 1.0);
}

inline Vector4d::Vector4d(const Quat & quat)
{
	mVec256 = dsseSetr((double)quat.getX(), (double)quat.getY(), (double)quat.getZ(), (double)quat.getW());
}

inline Vector4d::Vector4d(double scalar)
{
	mVec256 = dsseSet1(scalar);
}

inline Vector4d::Vector4d(const DoubleInVec & scalar)
{
	mVec256 = scalar.get256();
}

inline Vector4d::Vector4d(DSSEVec4 xyzw)
{
	mVec256 = xyzw;
}

//========================================= #TheForgeMathExtensionsBegin ================================================
//========================================= #TheForgeAnimationMathExtensionsBegin =======================================

inline const Vector4d Vector4d::fromVector4Int(const Vector4Int vecInt)
{
	SSEInt i;
	i.m128 = vecInt;
	return Vector4d(dsseFromIVec4(i));
}

//========================================= #TheForgeAnimationMathExtensionsEnd =======================================
//========================================= #TheForgeMathExtensionsEnd ================================================

inline const Vector4d Vector4d::xAxis()
{
	return Vector4d(dsseUnitVec1000());
}

inline const Vector4d Vector4d::yAxis()
{
	return Vector4d(dsseUnitVec0100());
}

inline const Vector4d Vector4d::zAxis()
{
	return Vector4d(dsseUnitVec0010());
}

inline const Vector4d Vector4d::wAxis()
{
	return Vector4d(dsseUnitVec0001());
}

inline DSSEVec4 Vector4d::get256() const
{
	return mVec256;
}

//========================================= #TheForgeMathExtensionsBegin ================================================
//========================================= #TheForgeAnimationMathExtensionsBegin =======================================

inline const Vector4d Vector4d::zero()
{
	return Vector4d(dsseSetZero());
}

inline const Vector4d Vector4d::one()
{
	return Vector4d(dsseSet1(1.0));
}

//========================================= #TheForgeAnimationMathExtensionsEnd =======================================
//========================================= #TheForgeMathExtensionsEnd ================================================

inline const Vector4d lerp(double t, const Vector4d & vec0, const Vector4d & vec1)
{
	return lerp(DoubleInVec(t), vec0, vec1);
}

inline const Vector4d lerp(const DoubleInVec & t, const Vector4d & vec0, const Vector4d & vec1)
{
	return (vec0 + ((vec1 - vec0) * t));
}

inline const Vector4d slerp(double t, const Vector4d & unitVec0, const Vector4d & unitVec1)
{
	return slerp(DoubleInVec(t), unitVec0, unitVec1);
}

inline const Vector4d slerp(const DoubleInVec & t, const Vector4d & unitVec0, const Vector4d & unitVec1)
{
	DSSEVec4 scales, scale0, scale1, cosAngle, angle, tttt, oneMinusT, angles, sines;
	cosAngle = dsseVecDot4(unitVec0.get256(), unitVec1.get256());
	DSSEVec4 selectMask = dsseGt(dsseSet1(VECTORMATH_SLERP_TOL), cosAngle);
	angle = dsseACosf(cosAngle);
	tttt = t.get256();
	oneMinusT = dsseSub(dsseSet1(1.0), tttt);
	angles = dsseMergeH(dsseSet1(1.0), tttt); // angles = 1, t, 1, t
	angles = dsseMergeH(angles, oneMinusT);   // angles = 1, 1-t, t, 1-t
	angles = dsseMul(angles, angle);
	sines = dsseSinf(angles);
	scales = dsseDiv(sines, dsseSplat(sines, 0));
	scale0 = dsseSelect(oneMinusT, dsseSplat(scales, 1), selectMask);
	scale1 = dsseSelect(tttt, dsseSplat(scales, 2), selectMask);
	return Vector4d(dsseMAdd(unitVec0.get256(), scale0, dsseMul(unitVec1.get256(), scale1)));
}

inline Vector4d & Vector4d::setXYZ(const Vector3d & vec)
{
	VECTORMATH_ALIGNED(unsigned long long sw[4]) = { 0, 0, 0, 0xFFFFFFFFFFFFFFFF };
	mVec256 = dsseSelect(vec.get256(), mVec256, sw);
	return *this;
}

inline const Vector3d Vector4d::getXYZ() const
{
	return Vector3d(mVec256);
}

inline Vector4d & Vector4d::setX(double _x)
{
	dsseVecSetElement(mVec256, _x, 0);
	return *this;
}

inline Vector4d & Vector4d::setX(const DoubleInVec & _x)
{
	mVec256 = dsseVecInsert(mVec256, _x.get256(), 0);
	return *this;
}

inline const DoubleInVec Vector4d::getX() const
{
	return DoubleInVec(mVec256, 0);
}

inline Vector4d & Vector4d::setY(double _y)
{
	dsseVecSetElement(mVec256, _y, 1);
	return *this;
}

inline Vector4d & Vector4d::setY(const DoubleInVec & _y)
{
	mVec256 = dsseVecInsert(mVec256, _y.get256(), 1);
	return *this;
}

inline const DoubleInVec Vector4d::getY() const
{
	return DoubleInVec(mVec256, 1);
}

inline Vector4d & Vector4d::setZ(double _z)
{
	dsseVecSetElement(mVec256, _z, 2);
	return *this;
}

inline Vector4d & Vector4d::setZ(const DoubleInVec & _z)
{
	mVec256 = dsseVecInsert(mVec256, _z.get256(), 2);
	return *this;
}

inline const DoubleInVec Vector4d::getZ() const
{
	return DoubleInVec(mVec256, 2);
}

inline Vector4d & Vector4d::setW(double _w)
{
	dsseVecSetElement(mVec256, _w, 3);
	return *this;
}

inline Vector4d & Vector4d::setW(const DoubleInVec & _w)
{
	mVec256 = dsseVecInsert(mVec256, _w.get256(), 3);
	return *this;
}

inline const DoubleInVec Vector4d::getW() const
{
	return DoubleInVec(mVec256, 3);
}

inline Vector4d & Vector4d::setElem(int idx, double value)
{
	dsseVecSetElement(mVec256, value, idx);
	return *this;
}

inline Vector4d & Vector4d::setElem(int idx, const DoubleInVec & value)
{
	mVec256 = dsseVecInsert(mVec256, value.get256(), idx);
	return *this;
}

inline const DoubleInVec Vector4d::getElem(int idx) const
{
	return DoubleInVec(mVec256, idx);
}

inline VecIdxd Vector4d::operator[](int idx)
{
	return VecIdxd(mVec256, idx);
}

inline const DoubleInVec Vector4d::operator[](int idx) const
{
	return DoubleInVec(mVec256, idx);
}

inline const Vector4d Vector4d::operator + (const Vector4d & vec) const
{
	return Vector4d(dsseAdd(mVec256, vec.mVec256));
}

inline const Vector4d Vector4d::operator - (const Vector4d & vec) const
{
	return Vector4d(dsseSub(mVec256, vec.mVec256));
}

inline const Vector4d Vector4d::operator * (double scalar) const
{
	return *this * DoubleInVec(scalar);
}

inline const Vector4d Vector4d::operator * (const DoubleInVec & scalar) const
{
	return Vector4d(dsseMul(mVec256, scalar.get256()));
}

inline Vector4d & Vector4d::operator += (const Vector4d & vec)
{
	*this = *this + vec;
	return *this;
}

inline Vector4d & Vector4d::operator -= (const Vector4d & vec)
{
	*this = *this - vec;
	return *this;
}

inline Vector4d & Vector4d::operator *= (double scalar)
{
	*this = *this * scalar;
	return *this;
}

inline Vector4d & Vector4d::operator *= (const DoubleInVec & scalar)
{
	*this = *this * scalar;
	return *this;
}

inline const Vector4d Vector4d::operator / (double scalar) const
{
	return *this / DoubleInVec(scalar);
}

inline const Vector4d Vector4d::operator / (const DoubleInVec & scalar) const
{
	return Vector4d(dsseDiv(mVec256, scalar.get256()));
}

inline Vector4d & Vector4d::operator /= (double scalar)
{
	*this = *this / scalar;
	return *this;
}

inline Vector4d & Vector4d::operator /= (const DoubleInVec & scalar)
{
	*this = *this / scalar;
	return *this;
}

inline const Vector4d Vector4d::operator - () const
{
	return Vector4d(dsseNegatef(mVec256));
}

inline const Vector4d operator * (double scalar, const Vector4d & vec)
{
	return DoubleInVec(scalar) * vec;
}

inline const Vector4d operator * (const DoubleInVec & scalar, const Vector4d & vec)
{
	return vec * scalar;
}

inline const Vector4d mulPerElem(const Vector4d & vec0, const Vector4d & vec1)
{
	return Vector4d(dsseMul(vec0.get256(), vec1.get256()));
}

inline const Vector4d divPerElem(const Vector4d & vec0, const Vector4d & vec1)
{
	return Vector4d(dsseDiv(vec0.get256(), vec1.get256()));
}

inline const Vector4d recipPerElem(const Vector4d & vec)
{
	return Vector4d(dsseRecipf(vec.get256()));
}

//========================================= #TheForgeMathExtensionsBegin ================================================
//========================================= #TheForgeAnimationMathExtensionsBegin =======================================

inline const Vector4d sqrtPerElem(const Vector4d & vec)
{
	return Vector4d(dsseSqrtf(vec.get256()));
}

inline const Vector4d rsqrtPerElem(const Vector4d & vec)
{
	return Vector4d(dsseRSqrtf(vec.get256()));
}

inline const Vector4d rcpEst(const Vector4d& v) {
	return Vector4d(dsseRecipf(v.get256()));
}

inline const Vector4d rSqrtEst(const Vector4d& v) {
	return Vector4d(dsseRecipf(v.get256()));
}

inline const Vector4d rSqrtEstNR(const Vector4d& v)
{
	return Vector4d(dsseNewtonrapsonRSqrtf(v.get256()));
}

inline const Vector4d aCos(const Vector4d& arg)
{
	return Vector4d(dsseACosf(arg.get256()));
}

//========================================= #TheForgeAnimationMathExtensionsEnd =======================================
//========================================= #TheForgeMathExtensionsEnd ================================================

inline const Vector4d absPerElem(const Vector4d & vec)
{
	return Vector4d(dsseFabsf(vec.get256()));
}

inline const Vector4d copySignPerElem(const Vector4d & vec0, const Vector4d & vec1)
{
	return Vector4d(dsseCopySign(vec0.get256(), vec1.get256()));
}

inline const Vector4d maxPerElem(const Vector4d & vec0, const Vector4d & vec1)
{
	return Vector4d(dsseMax(vec0.get256(), vec1.get256()));
}

inline const DoubleInVec maxElem(const Vector4d & vec)
{
	return DoubleInVec(dsseMax(
		dsseMax(dsseSplat(vec.get256(), 0), dsseSplat(vec.get256(), 1)),
		dsseMax(dsseSplat(vec.get256(), 2), dsseSplat(vec.get256(), 3)))
	);
}

inline const Vector4d minPerElem(const Vector4d & vec0, const Vector4d & vec1)
{
	return Vector4d(dsseMin(vec0.get256(), vec1.get256()));
}

inline const DoubleInVec minElem(const Vector4d & vec)
{
	return DoubleInVec(dsseMin(
		dsseMin(dsseSplat(vec.get256(), 0), dsseSplat(vec.get256(), 1)),
		dsseMin(dsseSplat(vec.get256(), 2), dsseSplat(vec.get256(), 3)))
	);
}

inline const DoubleInVec sum(const Vector4d & vec)
{
	return DoubleInVec(dsseAdd(
		dsseAdd(dsseSplat(vec.get256(), 0), dsseSplat(vec.get256(), 1)),
		dsseAdd(dsseSplat(vec.get256(), 2), dsseSplat(vec.get256(), 3)))
	);
}

inline const DoubleInVec dot(const Vector4d & vec0, const Vector4d & vec1)
{
	return DoubleInVec(dsseVecDot4(vec0.get256(), vec1.get256()), 0);
}

inline const DoubleInVec lengthSqr(const Vector4d & vec)
{
	return DoubleInVec(dsseVecDot4(vec.get256(), vec.get256()), 0);
}

inline const DoubleInVec length(const Vector4d & vec)
{
	return DoubleInVec(dsseSqrtf(dsseVecDot4(vec.get256(), vec.get256())), 0);
}

inline const Vector4d normalizeApprox(const Vector4d & vec)
{
	return Vector4d(dsseMul(vec.get256(), dsseRSqrtf(dsseVecDot4(vec.get256(), vec.get256()))));
}

inline const Vector4d normalize(const Vector4d & vec)
{
	return Vector4d(dsseMul(vec.get256(), dsseNewtonrapsonRSqrtf(dsseVecDot4(vec.get256(), vec.get256()))));
}

inline const Vector4d select(const Vector4d & vec0, const Vector4d & vec1, bool select1)
{
	return select(vec0, vec1, BoolInVec(select1));
}

inline const Vector4d select(const Vector4d & vec0, const Vector4d & vec1, const BoolInVec & select1)
{
	return Vector4d(dsseSelect(vec0.get256(), vec1.get256(), dsseFromBool(select1)));
}

//========================================= #TheForgeMathExtensionsBegin ================================================
//========================================= #TheForgeAnimationMathExtensionsBegin =======================================

inline const DSSEVec4 cmpEq(const Vector4d& a, const Vector4d& b) {
    return dsseEq(a.get256(), b.get256());
}

inline const DSSEVec4 cmpNotEq(const Vector4d& a, const Vector4d& b) {
    return dsseNe(a.get256(), b.get256());
}

inline const DSSEVec4 cmpLt(const Vector4d& a, const Vector4d& b) {
    return dsseLt(a.get256(), b.get256());
}

inline const DSSEVec4 cmpLe(const Vector4d& a, const Vector4d& b) {
    return dsseLe(a.get256(), b.get256());
}

inline const DSSEVec4 cmpGt(const Vector4d& a, const Vector4d& b) {
    return dsseGt(a.get256(), b.get256());
}

inline const DSSEVec4 cmpGe(const Vector4d& a, const Vector4d& b) {
    return dsseGe(a.get256(), b.get256());
}

inline const Vector4Int signBit(const Vector4d& v) {
	__m128 fv = dsseToFVec4(v.get256());
	return _mm_slli_epi32(_mm_srli_epi32(_mm_castps_si128(fv), 31), 31);
}

inline const Vector4d xorPerElem(const Vector4d& a, const Vector4Int b) {
	SSEInt c;
	c.m128 = _mm_xor_si128(dsseToIVec4(a.get256()), b);
	return Vector4d(dsseFromIVec4(c));
}

inline const Vector4d orPerElem(const Vector4d& a, const Vector4Int b) {
	SSEInt c;
	c.m128 = _mm_or_si128(dsseToIVec4(a.get256()), b);
	return Vector4d(dsseFromIVec4(c));
}

inline const Vector4d orPerElem(const Vector4d& a, const Vector4d& b) {
	return Vector4d(dsseOr(a.get256(), b.get256()));
}

inline const Vector4d andPerElem(const Vector4d& a, const Vector4Int b) {
	SSEInt c;
	c.m128 = _mm_and_si128(dsseToIVec4(a.get256()), b);
	return Vector4d(dsseFromIVec4(c));
}

inline const Vector4d halfTodouble(const Vector4Int vecInt) {
	const __m128i mask_nosign = _mm_set1_epi32(0x7fff);
	const __m128 magic = _mm_castsi128_ps(_mm_set1_epi32((254 - 15) << 23));
	const __m128i was_infnan = _mm_set1_epi32(0x7bff);
	const __m128 exp_infnan = _mm_castsi128_ps(_mm_set1_epi32(255 << 23));
	
	const __m128i expmant = _mm_and_si128(mask_nosign, vecInt);
	const __m128i shifted = _mm_slli_epi32(expmant, 13);
	const __m128 scaled = _mm_mul_ps(_mm_castsi128_ps(shifted), magic);
	const __m128i b_wasinfnan = _mm_cmpgt_epi32(expmant, was_infnan);
	const __m128i sign = _mm_slli_epi32(_mm_xor_si128(vecInt, expmant), 16);
	const __m128 infnanexp =_mm_and_ps(_mm_castsi128_ps(b_wasinfnan), exp_infnan);
	const __m128 sign_inf = _mm_or_ps(_mm_castsi128_ps(sign), infnanexp);
	SSEFloat c;
	c.m128 = _mm_or_ps(scaled, sign_inf);
	return Vector4d(dsseFromFVec4(c));
}

inline void transpose3x4(const Vector4d in[3], Vector4d out[4]) {
	const DSSEVec4 zero = dsseSetZero();
	const DSSEVec4 temp0 = dsseMergeH(in[0].get256(), in[1].get256());
	const DSSEVec4 temp1 = dsseMergeH(in[2].get256(), zero);
	const DSSEVec4 temp2 = dsseMergeL(in[0].get256(), in[1].get256());
	const DSSEVec4 temp3 = dsseMergeL(in[2].get256(), zero);
	out[0] = Vector4d(dsseMoveLH(temp0, temp1));
	out[1] = Vector4d(dsseMoveHL(temp1, temp0));
	out[2] = Vector4d(dsseMoveLH(temp2, temp3));
	out[3] = Vector4d(dsseMoveHL(temp3, temp2));
}

inline void transpose4x4(const Vector4d in[4], Vector4d out[4]) {
	const DSSEVec4 tmp0 = dsseMergeH(in[0].get256(), in[2].get256());
	const DSSEVec4 tmp1 = dsseMergeH(in[1].get256(), in[3].get256());
	const DSSEVec4 tmp2 = dsseMergeL(in[0].get256(), in[2].get256());
	const DSSEVec4 tmp3 = dsseMergeL(in[1].get256(), in[3].get256());
	out[0] = Vector4d(dsseMergeH(tmp0, tmp1));
	out[1] = Vector4d(dsseMergeL(tmp0, tmp1));
	out[2] = Vector4d(dsseMergeH(tmp2, tmp3));
	out[3] = Vector4d(dsseMergeL(tmp2, tmp3));
}

inline void transpose4x3(const Vector4d in[4], Vector4d out[4]) {
	const DSSEVec4 tmp0 = dsseMergeH(in[0].get256(), in[2].get256());
	const DSSEVec4 tmp1 = dsseMergeH(in[1].get256(), in[3].get256());
	const DSSEVec4 tmp2 = dsseMergeL(in[0].get256(), in[2].get256());
	const DSSEVec4 tmp3 = dsseMergeL(in[1].get256(), in[3].get256());
	out[0] = Vector4d(dsseMergeH(tmp0, tmp1));
	out[1] = Vector4d(dsseMergeL(tmp0, tmp1));
	out[2] = Vector4d(dsseMergeH(tmp2, tmp3));
}

inline void transpose16x16(const Vector4d in[16], Vector4d out[16]) {
	const DSSEVec4 tmp0 = dsseMergeH(in[0].get256(), in[2].get256());
	const DSSEVec4 tmp1 = dsseMergeH(in[1].get256(), in[3].get256());
	const DSSEVec4 tmp2 = dsseMergeL(in[0].get256(), in[2].get256());
	const DSSEVec4 tmp3 = dsseMergeL(in[1].get256(), in[3].get256());
	const DSSEVec4 tmp4 = dsseMergeH(in[4].get256(), in[6].get256());
	const DSSEVec4 tmp5 = dsseMergeH(in[5].get256(), in[7].get256());
	const DSSEVec4 tmp6 = dsseMergeL(in[4].get256(), in[6].get256());
	const DSSEVec4 tmp7 = dsseMergeL(in[5].get256(), in[7].get256());
	const DSSEVec4 tmp8 = dsseMergeH(in[8].get256(), in[10].get256());
	const DSSEVec4 tmp9 = dsseMergeH(in[9].get256(), in[11].get256());
	const DSSEVec4 tmp10 = dsseMergeL(in[8].get256(), in[10].get256());
	const DSSEVec4 tmp11 = dsseMergeL(in[9].get256(), in[11].get256());
	const DSSEVec4 tmp12 = dsseMergeH(in[12].get256(), in[14].get256());
	const DSSEVec4 tmp13 = dsseMergeH(in[13].get256(), in[15].get256());
	const DSSEVec4 tmp14 = dsseMergeL(in[12].get256(), in[14].get256());
	const DSSEVec4 tmp15 = dsseMergeL(in[13].get256(), in[15].get256());
	out[0] = Vector4d(dsseMergeH(tmp0, tmp1));
	out[1] = Vector4d(dsseMergeH(tmp4, tmp5));
	out[2] = Vector4d(dsseMergeH(tmp8, tmp9));
	out[3] = Vector4d(dsseMergeH(tmp12, tmp13));
	out[4] = Vector4d(dsseMergeL(tmp0, tmp1));
	out[5] = Vector4d(dsseMergeL(tmp4, tmp5));
	out[6] = Vector4d(dsseMergeL(tmp8, tmp9));
	out[7] = Vector4d(dsseMergeL(tmp12, tmp13));
	out[8] = Vector4d(dsseMergeH(tmp2, tmp3));
	out[9] = Vector4d(dsseMergeH(tmp6, tmp7));
	out[10] = Vector4d(dsseMergeH(tmp10, tmp11));
	out[11] = Vector4d(dsseMergeH(tmp14, tmp15));
	out[12] = Vector4d(dsseMergeL(tmp2, tmp3));
	out[13] = Vector4d(dsseMergeL(tmp6, tmp7));
	out[14] = Vector4d(dsseMergeL(tmp10, tmp11));
	out[15] = Vector4d(dsseMergeL(tmp14, tmp15));
}

inline void storePtrU(const Vector4d& v, double* d) {
	_mm_storeu_pd(d + 0, v.get256().xy);
	_mm_storeu_pd(d + 2, v.get256().zw);
}

inline void store3PtrU(const Vector4d& v, double* d) {
	_mm_storeu_pd(d, v.get256().xy);
	d[2] = v.get256().d[2];
}

//========================================= #TheForgeAnimationMathExtensionsEnd =======================================
//========================================= #TheForgeMathExtensionsEnd ================================================

#ifdef VECTORMATH_DEBUG

inline void print(const Vector4d & vec)
{
	std::printf("( %f %f %f %f )\n", vec.get256().d[0], vec.get256().d[1], vec.get256().d[2], vec.get256().d[3]);
}

inline void print(const Vector4d & vec, const char * name)
{
	std::printf("%s: ( %f %f %f %f )\n", name, vec.get256().d[0], vec.get256().d[1], vec.get256().d[2], vec.get256().d[3]);
}

#endif // VECTORMATH_DEBUG*/

// ========================================================
// Point3
// ========================================================

inline Point3::Point3(float _x, float _y, float _z)
{
    mVec128 = _mm_setr_ps(_x, _y, _z, 1.0f);
}

inline Point3::Point3(const FloatInVec & _x, const FloatInVec & _y, const FloatInVec & _z)
{
    mVec128 = _mm_unpacklo_ps(_mm_unpacklo_ps(_x.get128(), _z.get128()), _y.get128());
}

inline Point3::Point3(const Vector3 & vec)
{
    mVec128 = vec.get128();
    setW(1);
}

inline Point3::Point3(const Vector4 & vec)
{
	mVec128 = vec.get128();
}

inline Point3::Point3(float scalar)
{
    mVec128 = FloatInVec(scalar).get128();
    setW(1);
}

inline Point3::Point3(const FloatInVec & scalar)
{
    mVec128 = scalar.get128();
    setW(1);
}

inline Point3::Point3(__m128 vf4)
{
    mVec128 = vf4;
}

inline const Point3 lerp(float t, const Point3 & pnt0, const Point3 & pnt1)
{
    return lerp(FloatInVec(t), pnt0, pnt1);
}

inline const Point3 lerp(const FloatInVec & t, const Point3 & pnt0, const Point3 & pnt1)
{
    return (pnt0 + ((pnt1 - pnt0) * t));
}

inline __m128 Point3::get128() const
{
    return mVec128;
}

inline void storeXYZ(const Point3 & pnt, __m128 * quad)
{
    __m128 dstVec = *quad;
    VECTORMATH_ALIGNED(unsigned int sw[4]) = { 0, 0, 0, 0xFFFFFFFF };
    dstVec = sseSelect(pnt.get128(), dstVec, sw);
    *quad = dstVec;
}

inline void loadXYZArray(Point3 & pnt0, Point3 & pnt1, Point3 & pnt2, Point3 & pnt3, const __m128 * threeQuads)
{
    const float * quads = (const float *)threeQuads;
    pnt0 = Point3(_mm_load_ps(quads));
    pnt1 = Point3(_mm_loadu_ps(quads + 3));
    pnt2 = Point3(_mm_loadu_ps(quads + 6));
    pnt3 = Point3(_mm_loadu_ps(quads + 9));
}

inline void storeXYZArray(const Point3 & pnt0, const Point3 & pnt1, const Point3 & pnt2, const Point3 & pnt3, __m128 * threeQuads)
{
    __m128 xxxx = _mm_shuffle_ps(pnt1.get128(), pnt1.get128(), _MM_SHUFFLE(0, 0, 0, 0));
    __m128 zzzz = _mm_shuffle_ps(pnt2.get128(), pnt2.get128(), _MM_SHUFFLE(2, 2, 2, 2));
    VECTORMATH_ALIGNED(unsigned int xsw[4]) = { 0, 0, 0, 0xFFFFFFFF };
    VECTORMATH_ALIGNED(unsigned int zsw[4]) = { 0xFFFFFFFF, 0, 0, 0 };
    threeQuads[0] = sseSelect(pnt0.get128(), xxxx, xsw);
    threeQuads[1] = _mm_shuffle_ps(pnt1.get128(), pnt2.get128(), _MM_SHUFFLE(1, 0, 2, 1));
    threeQuads[2] = sseSelect(_mm_shuffle_ps(pnt3.get128(), pnt3.get128(), _MM_SHUFFLE(2, 1, 0, 3)), zzzz, zsw);
}

inline Point3 & Point3::setX(float _x)
{
    sseVecSetElement(mVec128, _x, 0);
    return *this;
}

inline Point3 & Point3::setX(const FloatInVec & _x)
{
    mVec128 = sseVecInsert(mVec128, _x.get128(), 0);
    return *this;
}

inline const FloatInVec Point3::getX() const
{
    return FloatInVec(mVec128, 0);
}

inline Point3 & Point3::setY(float _y)
{
    sseVecSetElement(mVec128, _y, 1);
    return *this;
}

inline Point3 & Point3::setY(const FloatInVec & _y)
{
    mVec128 = sseVecInsert(mVec128, _y.get128(), 1);
    return *this;
}

inline const FloatInVec Point3::getY() const
{
    return FloatInVec(mVec128, 1);
}

inline Point3 & Point3::setZ(float _z)
{
    sseVecSetElement(mVec128, _z, 2);
    return *this;
}

inline Point3 & Point3::setZ(const FloatInVec & _z)
{
    mVec128 = sseVecInsert(mVec128, _z.get128(), 2);
    return *this;
}

inline const FloatInVec Point3::getZ() const
{
    return FloatInVec(mVec128, 2);
}

inline Point3 & Point3::setW(float _w)
{
    sseVecSetElement(mVec128, _w, 3);
    return *this;
}

inline Point3 & Point3::setW(const FloatInVec & _w)
{
    mVec128 = sseVecInsert(mVec128, _w.get128(), 3);
    return *this;
}

inline const FloatInVec Point3::getW() const
{
    return FloatInVec(mVec128, 3);
}

inline Point3 & Point3::setElem(int idx, float value)
{
    sseVecSetElement(mVec128, value, idx);
    return *this;
}

inline Point3 & Point3::setElem(int idx, const FloatInVec & value)
{
    mVec128 = sseVecInsert(mVec128, value.get128(), idx);
    return *this;
}

inline const FloatInVec Point3::getElem(int idx) const
{
    return FloatInVec(mVec128, idx);
}

inline VecIdx Point3::operator[](int idx)
{
    return VecIdx(mVec128, idx);
}

inline const FloatInVec Point3::operator[](int idx) const
{
    return FloatInVec(mVec128, idx);
}

inline const Vector3 Point3::operator - (const Point3 & pnt) const
{
    return Vector3(_mm_sub_ps(mVec128, pnt.mVec128));
}

inline const Point3 Point3::operator + (const Vector3 & vec) const
{
    return Point3(_mm_add_ps(mVec128, vec.get128()));
}

inline const Point3 Point3::operator - (const Vector3 & vec) const
{
    return Point3(_mm_sub_ps(mVec128, vec.get128()));
}

inline Point3 & Point3::operator += (const Vector3 & vec)
{
    *this = *this + vec;
    return *this;
}

inline Point3 & Point3::operator -= (const Vector3 & vec)
{
    *this = *this - vec;
    return *this;
}

inline const Point3 mulPerElem(const Point3 & pnt0, const Point3 & pnt1)
{
    return Point3(_mm_mul_ps(pnt0.get128(), pnt1.get128()));
}

inline const Point3 divPerElem(const Point3 & pnt0, const Point3 & pnt1)
{
    return Point3(_mm_div_ps(pnt0.get128(), pnt1.get128()));
}

inline const Point3 recipPerElem(const Point3 & pnt)
{
    return Point3(_mm_rcp_ps(pnt.get128()));
}

inline const Point3 absPerElem(const Point3 & pnt)
{
    return Point3(sseFabsf(pnt.get128()));
}

inline const Point3 copySignPerElem(const Point3 & pnt0, const Point3 & pnt1)
{
    const __m128 vmask = sseUintToM128(0x7FFFFFFF);
    return Point3(_mm_or_ps(
        _mm_and_ps(vmask, pnt0.get128()),      // Value
        _mm_andnot_ps(vmask, pnt1.get128()))); // Signs
}

inline const Point3 maxPerElem(const Point3 & pnt0, const Point3 & pnt1)
{
    return Point3(_mm_max_ps(pnt0.get128(), pnt1.get128()));
}

inline const FloatInVec maxElem(const Point3 & pnt)
{
    return FloatInVec(_mm_max_ps(_mm_max_ps(sseSplat(pnt.get128(), 0), sseSplat(pnt.get128(), 1)), sseSplat(pnt.get128(), 2)));
}

inline const Point3 minPerElem(const Point3 & pnt0, const Point3 & pnt1)
{
    return Point3(_mm_min_ps(pnt0.get128(), pnt1.get128()));
}

inline const FloatInVec minElem(const Point3 & pnt)
{
    return FloatInVec(_mm_min_ps(_mm_min_ps(sseSplat(pnt.get128(), 0), sseSplat(pnt.get128(), 1)), sseSplat(pnt.get128(), 2)));
}

inline const FloatInVec sum(const Point3 & pnt)
{
    return FloatInVec(_mm_add_ps(_mm_add_ps(sseSplat(pnt.get128(), 0), sseSplat(pnt.get128(), 1)), sseSplat(pnt.get128(), 2)));
}

inline const Point3 scale(const Point3 & pnt, float scaleVal)
{
    return scale(pnt, FloatInVec(scaleVal));
}

inline const Point3 scale(const Point3 & pnt, const FloatInVec & scaleVal)
{
    return mulPerElem(pnt, Point3(scaleVal));
}

inline const Point3 scale(const Point3 & pnt, const Vector3 & scaleVec)
{
    return mulPerElem(pnt, Point3(scaleVec));
}

inline const Point3 normalize(const Point3 & vec)
{
    return Point3(_mm_mul_ps(vec.get128(), sseNewtonrapsonRSqrtf(sseVecDot3(vec.get128(), vec.get128()))));
}

inline const FloatInVec projection(const Point3 & pnt, const Vector3 & unitVec)
{
    return FloatInVec(sseVecDot3(pnt.get128(), unitVec.get128()), 0);
}

inline const FloatInVec distSqrFromOrigin(const Point3 & pnt)
{
    return lengthSqr(Vector3(pnt));
}

inline const FloatInVec distFromOrigin(const Point3 & pnt)
{
    return length(Vector3(pnt));
}

inline const FloatInVec distSqr(const Point3 & pnt0, const Point3 & pnt1)
{
    return lengthSqr((pnt1 - pnt0));
}

inline const FloatInVec dist(const Point3 & pnt0, const Point3 & pnt1)
{
    return length((pnt1 - pnt0));
}

inline const FloatInVec distFromPlane(const Point3& pnt, const Vector4& plane)
{
	return dot(Vector4(Vector3(pnt), 1.0f), plane).abs();
}

inline const Point3 select(const Point3 & pnt0, const Point3 & pnt1, bool select1)
{
    return select(pnt0, pnt1, BoolInVec(select1));
}

inline const Point3 select(const Point3 & pnt0, const Point3 & pnt1, const BoolInVec & select1)
{
    return Point3(sseSelect(pnt0.get128(), pnt1.get128(), select1.get128()));
}

inline bool Point3::similar(const Point3& other, float threshold) const
{
    return (lengthSqr((*this - other)) < FloatInVec(threshold));
}

#ifdef VECTORMATH_DEBUG

inline void print(const Point3 & pnt)
{
    SSEFloat tmp;
    tmp.m128 = pnt.get128();
    std::printf("( %f %f %f )\n", tmp.f[0], tmp.f[1], tmp.f[2]);
}

inline void print(const Point3 & pnt, const char * name)
{
    SSEFloat tmp;
    tmp.m128 = pnt.get128();
    std::printf("%s: ( %f %f %f )\n", name, tmp.f[0], tmp.f[1], tmp.f[2]);
}

#endif // VECTORMATH_DEBUG

//========================================= #TheForgeMathExtensionsBegin ================================================
//========================================= #TheForgeAnimationMathExtensionsBegin =======================================

#define SSE_SELECT_I(_b, _true, _false) \
  _mm_xor_si128(_false, _mm_and_si128(_b, _mm_xor_si128(_true, _false)))

#define SSE_SPLAT_I(_v, _i)                                               \
  _mm_castps_si128(_mm_shuffle_ps(_mm_castsi128_ps(_v), _mm_castsi128_ps(_v), \
                                  _MM_SHUFFLE(_i, _i, _i, _i)))

#define SSE_SPLATF_I(_v, _i)                                               \
  _mm_shuffle_ps(_mm_castsi128_ps(_v), _mm_castsi128_ps(_v), \
                                  _MM_SHUFFLE(_i, _i, _i, _i))

// ========================================================
// Vector4Int
// ========================================================

namespace vector4int {

inline Vector4Int zero() { return _mm_setzero_si128(); }

inline Vector4Int one() {
  const __m128i zero = _mm_setzero_si128();
  const __m128i ffff = _mm_cmpeq_epi32(zero, zero);
  return _mm_srli_epi32(ffff, 31);
}

inline Vector4Int x_axis() {
  const __m128i zero = _mm_setzero_si128();
  const __m128i ffff = _mm_cmpeq_epi32(zero, zero);
  return _mm_srli_si128(_mm_srli_epi32(ffff, 31), 12);
}

inline Vector4Int y_axis() {
  const __m128i zero = _mm_setzero_si128();
  const __m128i ffff = _mm_cmpeq_epi32(zero, zero);
  return _mm_slli_si128(_mm_srli_si128(_mm_srli_epi32(ffff, 31), 12), 4);
}

inline Vector4Int z_axis() {
  const __m128i zero = _mm_setzero_si128();
  const __m128i ffff = _mm_cmpeq_epi32(zero, zero);
  return _mm_slli_si128(_mm_srli_si128(_mm_srli_epi32(ffff, 31), 12), 8);
}

inline Vector4Int w_axis() {
  const __m128i zero = _mm_setzero_si128();
  const __m128i ffff = _mm_cmpeq_epi32(zero, zero);
  return _mm_slli_si128(_mm_srli_epi32(ffff, 31), 12);
}

inline Vector4Int all_true() {
  return _mm_cmpeq_epi32(_mm_setzero_si128(), _mm_setzero_si128());
}

inline Vector4Int all_false() { return _mm_setzero_si128(); }

inline Vector4Int mask_sign() {
  const __m128i ffff =
      _mm_cmpeq_epi32(_mm_setzero_si128(), _mm_setzero_si128());
  return _mm_slli_epi32(ffff, 31);
}

inline Vector4Int mask_not_sign() {
  const __m128i ffff =
      _mm_cmpeq_epi32(_mm_setzero_si128(), _mm_setzero_si128());
  return _mm_srli_epi32(ffff, 1);
}

inline Vector4Int mask_ffff() {
  return _mm_cmpeq_epi32(_mm_setzero_si128(), _mm_setzero_si128());
}

inline Vector4Int mask_0000() { return _mm_setzero_si128(); }

inline Vector4Int mask_fff0() {
  const __m128i zero = _mm_setzero_si128();
  const __m128i ffff = _mm_cmpeq_epi32(zero, zero);
  return _mm_srli_si128(ffff, 4);
}

inline Vector4Int mask_f000() {
  const __m128i zero = _mm_setzero_si128();
  const __m128i ffff = _mm_cmpeq_epi32(zero, zero);
  return _mm_srli_si128(ffff, 12);
}

inline Vector4Int mask_0f00() {
  const __m128i zero = _mm_setzero_si128();
  const __m128i ffff = _mm_cmpeq_epi32(zero, zero);
  return _mm_srli_si128(_mm_slli_si128(ffff, 12), 8);
}

inline Vector4Int mask_00f0() {
  const __m128i zero = _mm_setzero_si128();
  const __m128i ffff = _mm_cmpeq_epi32(zero, zero);
  return _mm_srli_si128(_mm_slli_si128(ffff, 12), 4);
}

inline Vector4Int mask_000f() {
  const __m128i zero = _mm_setzero_si128();
  const __m128i ffff = _mm_cmpeq_epi32(zero, zero);
  return _mm_slli_si128(ffff, 12);
}

inline Vector4Int Load(int _x, int _y, int _z, int _w) {
  return _mm_set_epi32(_w, _z, _y, _x);
}

inline Vector4Int LoadX(int _x) { return _mm_set_epi32(0, 0, 0, _x); }

inline Vector4Int Load1(int _x) { return _mm_set1_epi32(_x); }

inline Vector4Int Load(bool _x, bool _y, bool _z, bool _w) {
  return _mm_sub_epi32(_mm_setzero_si128(), _mm_set_epi32(_w, _z, _y, _x));
}

inline Vector4Int LoadX(bool _x) {
  return _mm_sub_epi32(_mm_setzero_si128(), _mm_set_epi32(0, 0, 0, _x));
}

inline Vector4Int Load1(bool _x) {
  return _mm_sub_epi32(_mm_setzero_si128(), _mm_set1_epi32(_x));
}

inline Vector4Int LoadPtr(const int* _i) {
  return _mm_load_si128(reinterpret_cast<const __m128i*>(_i)); //-V1032
}

inline Vector4Int LoadXPtr(const int* _i) {
  return _mm_cvtsi32_si128(*_i);
}

inline Vector4Int Load1Ptr(const int* _i) {
  return _mm_shuffle_epi32(
      _mm_loadl_epi64(reinterpret_cast<const __m128i*>(_i)), //-V1032
      _MM_SHUFFLE(0, 0, 0, 0));
}

inline Vector4Int Load2Ptr(const int* _i) {
  return _mm_loadl_epi64(reinterpret_cast<const __m128i*>(_i)); //-V1032
}

inline Vector4Int Load3Ptr(const int* _i) {
  return _mm_set_epi32(0, _i[2], _i[1], _i[0]);
}

inline Vector4Int LoadPtrU(const int* _i) {
  return _mm_loadu_si128(reinterpret_cast<const __m128i*>(_i)); //-V1032
}

inline Vector4Int LoadXPtrU(const int* _i) {
  return _mm_cvtsi32_si128(*_i);
}

inline Vector4Int Load1PtrU(const int* _i) {
  return _mm_set1_epi32(*_i);
}

inline Vector4Int Load2PtrU(const int* _i) {
  return _mm_set_epi32(0, 0, _i[1], _i[0]);
}

inline Vector4Int Load3PtrU(const int* _i) {
  return _mm_set_epi32(0, _i[2], _i[1], _i[0]);
}

inline Vector4Int FromFloatRound(const Vector4& _f) {
  return _mm_cvtps_epi32(_f.get128());
}

inline Vector4Int FromFloatTrunc(const Vector4& _f) {
  return _mm_cvttps_epi32(_f.get128());
}
}  // namespace vector4int

inline int GetX(const Vector4Int _v) { return _mm_cvtsi128_si32(_v); }

inline int GetY(const Vector4Int _v) {
  return _mm_cvtsi128_si32(SSE_SPLAT_I(_v, 1));
}

inline int GetZ(const Vector4Int _v) {
  return _mm_cvtsi128_si32(_mm_unpackhi_epi32(_v, _v));
}

inline int GetW(const Vector4Int _v) {
  return _mm_cvtsi128_si32(SSE_SPLAT_I(_v, 3));
}

inline Vector4Int SetX(const Vector4Int _v, int _i) {
  return _mm_castps_si128(
      _mm_move_ss(_mm_castsi128_ps(_v), _mm_castsi128_ps(_mm_set1_epi32(_i))));
}

inline Vector4Int SetY(const Vector4Int _v, int _i) {
  const __m128 i = _mm_castsi128_ps(_mm_set1_epi32(_i));
  const __m128 v = _mm_castsi128_ps(_v);
  const __m128 yxzw = _mm_shuffle_ps(v, v, _MM_SHUFFLE(3, 2, 0, 1));
  const __m128 fxzw = _mm_move_ss(yxzw, i);
  return _mm_castps_si128(_mm_shuffle_ps(fxzw, fxzw, _MM_SHUFFLE(3, 2, 0, 1)));
}

inline Vector4Int SetZ(const Vector4Int _v, int _i) {
  const __m128 i = _mm_castsi128_ps(_mm_set1_epi32(_i));
  const __m128 v = _mm_castsi128_ps(_v);
  const __m128 yxzw = _mm_shuffle_ps(v, v, _MM_SHUFFLE(3, 0, 1, 2));
  const __m128 fxzw = _mm_move_ss(yxzw, i);
  return _mm_castps_si128(_mm_shuffle_ps(fxzw, fxzw, _MM_SHUFFLE(3, 0, 1, 2)));
}

inline Vector4Int SetW(const Vector4Int _v, int _i) {
  const __m128 i = _mm_castsi128_ps(_mm_set1_epi32(_i));
  const __m128 v = _mm_castsi128_ps(_v);
  const __m128 yxzw = _mm_shuffle_ps(v, v, _MM_SHUFFLE(0, 2, 1, 3));
  const __m128 fxzw = _mm_move_ss(yxzw, i);
  return _mm_castps_si128(_mm_shuffle_ps(fxzw, fxzw, _MM_SHUFFLE(0, 2, 1, 3)));
}

inline Vector4Int SetI(const Vector4Int _v, int _ith, int _i) {
  union {
    Vector4Int ret;
    int af[4];
  } u = {_v};
  u.af[_ith] = _i;
  return u.ret;
}

inline void StorePtr(const Vector4Int _v, int* _i) {
  _mm_store_si128(reinterpret_cast<__m128i*>(_i), _v); //-V1032
}

inline void Store1Ptr(const Vector4Int _v, int* _i) {
  *_i = _mm_cvtsi128_si32(_v);
}

inline void Store2Ptr(const Vector4Int _v, int* _i) {
  _i[0] = _mm_cvtsi128_si32(_v);
  _i[1] = _mm_cvtsi128_si32(SSE_SPLAT_I(_v, 1));
}

inline void Store3Ptr(const Vector4Int _v, int* _i) {
  _i[0] = _mm_cvtsi128_si32(_v);
  _i[1] = _mm_cvtsi128_si32(SSE_SPLAT_I(_v, 1));
  _i[2] = _mm_cvtsi128_si32(_mm_unpackhi_epi32(_v, _v));
}

inline void StorePtrU(const Vector4Int _v, int* _i) {
  _mm_storeu_si128(reinterpret_cast<__m128i*>(_i), _v); //-V1032
}

inline void Store1PtrU(const Vector4Int _v, int* _i) {
  *_i = _mm_cvtsi128_si32(_v);
}

inline void Store2PtrU(const Vector4Int _v, int* _i) {
  _i[0] = _mm_cvtsi128_si32(_v);
  _i[1] = _mm_cvtsi128_si32(SSE_SPLAT_I(_v, 1));
}

inline void Store3PtrU(const Vector4Int _v, int* _i) {
  _i[0] = _mm_cvtsi128_si32(_v);
  _i[1] = _mm_cvtsi128_si32(SSE_SPLAT_I(_v, 1));
  _i[2] = _mm_cvtsi128_si32(_mm_unpackhi_epi32(_v, _v));
}

inline Vector4Int SplatX(const Vector4Int _a) { return SSE_SPLAT_I(_a, 0); }

inline Vector4Int SplatY(const Vector4Int _a) { return SSE_SPLAT_I(_a, 1); }

inline Vector4Int SplatZ(const Vector4Int _a) { return SSE_SPLAT_I(_a, 2); }

inline Vector4Int SplatW(const Vector4Int _a) { return SSE_SPLAT_I(_a, 3); }

inline int MoveMask(const Vector4Int _v) {
  return _mm_movemask_ps(_mm_castsi128_ps(_v));
}

inline bool AreAllTrue(const Vector4Int _v) {
  return _mm_movemask_ps(_mm_castsi128_ps(_v)) == 0xf;
}

inline bool AreAllTrue3(const Vector4Int _v) {
  return (_mm_movemask_ps(_mm_castsi128_ps(_v)) & 0x7) == 0x7;
}

inline bool AreAllTrue2(const Vector4Int _v) {
  return (_mm_movemask_ps(_mm_castsi128_ps(_v)) & 0x3) == 0x3;
}

inline bool AreAllTrue1(const Vector4Int _v) {
  return (_mm_movemask_ps(_mm_castsi128_ps(_v)) & 0x1) == 0x1;
}

inline bool AreAllFalse(const Vector4Int _v) {
  return _mm_movemask_ps(_mm_castsi128_ps(_v)) == 0;
}

inline bool AreAllFalse3(const Vector4Int _v) {
  return (_mm_movemask_ps(_mm_castsi128_ps(_v)) & 0x7) == 0;
}

inline bool AreAllFalse2(const Vector4Int _v) {
  return (_mm_movemask_ps(_mm_castsi128_ps(_v)) & 0x3) == 0;
}

inline bool AreAllFalse1(const Vector4Int _v) {
  return (_mm_movemask_ps(_mm_castsi128_ps(_v)) & 0x1) == 0;
}

inline Vector4Int HAdd2(const Vector4Int _v) {
  const __m128i hadd = _mm_add_epi32(_v, SSE_SPLAT_I(_v, 1));
  return _mm_castps_si128(
      _mm_move_ss(_mm_castsi128_ps(_v), _mm_castsi128_ps(hadd)));
}

inline Vector4Int HAdd3(const Vector4Int _v) {
  const __m128i hadd = _mm_add_epi32(_mm_add_epi32(_v, SSE_SPLAT_I(_v, 1)),
                                     _mm_unpackhi_epi32(_v, _v));
  return _mm_castps_si128(
      _mm_move_ss(_mm_castsi128_ps(_v), _mm_castsi128_ps(hadd)));
}

inline Vector4Int HAdd4(const Vector4Int _v) {
  const __m128 v = _mm_castsi128_ps(_v);
  const __m128i haddxyzw =
      _mm_add_epi32(_v, _mm_castps_si128(_mm_movehl_ps(v, v)));
  return _mm_castps_si128(_mm_move_ss(
      v,
      _mm_castsi128_ps(_mm_add_epi32(haddxyzw, SSE_SPLAT_I(haddxyzw, 1)))));
}

inline Vector4Int Abs(const Vector4Int _v) {
  const __m128i zero = _mm_setzero_si128();
  return SSE_SELECT_I(_mm_cmplt_epi32(_v, zero), _mm_sub_epi32(zero, _v),
                          _v);
}

inline Vector4Int Sign(const Vector4Int _v) {
  return _mm_slli_epi32(_mm_srli_epi32(_v, 31), 31);
}

inline Vector4Int Min(const Vector4Int _a, const Vector4Int _b) {
  // SSE4 _mm_min_epi32
  return SSE_SELECT_I(_mm_cmplt_epi32(_a, _b), _a, _b);
}

inline Vector4Int Max(const Vector4Int _a, const Vector4Int _b) {
  // SSE4 _mm_max_epi32
  return SSE_SELECT_I(_mm_cmpgt_epi32(_a, _b), _a, _b);
}

inline Vector4Int Min0(const Vector4Int _v) {
  // SSE4 _mm_min_epi32
  const __m128i zero = _mm_setzero_si128();
  return SSE_SELECT_I(_mm_cmplt_epi32(zero, _v), zero, _v);
}

inline Vector4Int Max0(const Vector4Int _v) {
  // SSE4 _mm_max_epi32
  const __m128i zero = _mm_setzero_si128();
  return SSE_SELECT_I(_mm_cmpgt_epi32(zero, _v), zero, _v);
}

inline Vector4Int Clamp(const Vector4Int _a, const Vector4Int _v, const Vector4Int _b) {
  // SSE4 _mm_min_epi32/_mm_max_epi32
  const __m128i min = SSE_SELECT_I(_mm_cmplt_epi32(_v, _b), _v, _b);
  return SSE_SELECT_I(_mm_cmpgt_epi32(_a, min), _a, min);
}

inline Vector4Int Select(const Vector4Int _b, const Vector4Int _true, const Vector4Int _false) {
  return SSE_SELECT_I(_b, _true, _false);
}

inline Vector4Int And(const Vector4Int _a, const Vector4Int _b) {
  return _mm_and_si128(_a, _b);
}

inline Vector4Int And(const Vector4Int& a, const BoolInVec b) {
  return _mm_and_si128(a, _mm_castps_si128(b.get128()));
}

inline Vector4Int Or(const Vector4Int _a, const Vector4Int _b) {
  return _mm_or_si128(_a, _b);
}

inline Vector4Int Xor(const Vector4Int _a, const Vector4Int _b) {
  return _mm_xor_si128(_a, _b);
}

inline Vector4Int Not(const Vector4Int _v) {
  return _mm_andnot_si128(
      _v, _mm_cmpeq_epi32(_mm_setzero_si128(), _mm_setzero_si128()));
}

inline Vector4Int ShiftL(const Vector4Int _v, int _bits) {
  return _mm_slli_epi32(_v, _bits);
}

inline Vector4Int ShiftR(const Vector4Int _v, int _bits) {
  return _mm_srai_epi32(_v, _bits);
}

inline Vector4Int ShiftRu(const Vector4Int _v, int _bits) {
  return _mm_srli_epi32(_v, _bits);
}

inline Vector4Int CmpEq(const Vector4Int _a, const Vector4Int _b) {
  return _mm_cmpeq_epi32(_a, _b);
}

inline Vector4Int CmpNe(const Vector4Int _a, const Vector4Int _b) {
  return _mm_castps_si128(
      _mm_cmpneq_ps(_mm_castsi128_ps(_a), _mm_castsi128_ps(_b)));
}

inline Vector4Int CmpLt(const Vector4Int _a, const Vector4Int _b) {
  return _mm_cmplt_epi32(_a, _b);
}

inline Vector4Int CmpLe(const Vector4Int _a, const Vector4Int _b) {
  return _mm_andnot_si128(
      _mm_cmpgt_epi32(_a, _b),
      _mm_cmpeq_epi32(_mm_setzero_si128(), _mm_setzero_si128()));
}

inline Vector4Int CmpGt(const Vector4Int _a, const Vector4Int _b) {
  return _mm_cmpgt_epi32(_a, _b);
}

inline Vector4Int CmpGe(const Vector4Int _a, const Vector4Int _b) {
  return _mm_andnot_si128(
      _mm_cmplt_epi32(_a, _b),
      _mm_cmpeq_epi32(_mm_setzero_si128(), _mm_setzero_si128()));
}

//========================================= #TheForgeAnimationMathExtensionsEnd =======================================
// ========================================================
// IVecIdx
// ========================================================

#ifdef VECTORMATH_NO_SCALAR_CAST
inline int IVecIdx::getAsInt() const
#else
inline IVecIdx::operator int() const
#endif
{
	return ((int *)&ref)[i];
}

inline int IVecIdx::operator = (int scalar)
{
	((int *)&(ref))[i] = scalar;
	return scalar;
}

inline int IVecIdx::operator = (const IVecIdx & scalar)
{
	((int *)&(ref))[i] = ((int *)&(scalar.ref))[scalar.i];
	return *this;
}

inline int IVecIdx::operator *= (int scalar)
{
	((int *)&(ref))[i] *= scalar;
	return *this;
}

inline int IVecIdx::operator /= (int scalar)
{
	((int *)&(ref))[i] /= scalar;
	return *this;
}

inline int IVecIdx::operator += (int scalar)
{
	((int *)&(ref))[i] += scalar;
	return *this;
}

inline int IVecIdx::operator -= (int scalar)
{
	((int *)&(ref))[i] -= scalar;
	return *this;
}

// ========================================================
// IVector3
// ========================================================

inline IVector3::IVector3(int _x, int _y, int _z)
{
	mVec128 = _mm_setr_epi32(_x, _y, _z, 0);
}

inline IVector3::IVector3(int scalar)
{
	mVec128 = _mm_setr_epi32(scalar, scalar, scalar, 0);
}

inline IVector3::IVector3(__m128i vi4)
{
	mVec128 = vi4;
}

inline const IVector3 IVector3::xAxis()
{
	return IVector3(1, 0, 0);
}

inline const IVector3 IVector3::yAxis()
{
	return IVector3(0, 1, 0);
}

inline const IVector3 IVector3::zAxis()
{
	return IVector3(0, 0, 1);
}

inline __m128i IVector3::get128() const
{
	return mVec128;
}

inline IVector3 & IVector3::setX(int _x)
{
	((int*)(&mVec128))[0] = _x;
	return *this;
}

inline const int IVector3::getX() const
{
	return ((int*)&mVec128)[0];
}

inline IVector3 & IVector3::setY(int _y)
{
	((int*)(&mVec128))[1] = _y;
	return *this;
}

inline const int IVector3::getY() const
{
	return ((int*)&mVec128)[1];
}

inline IVector3 & IVector3::setZ(int _z)
{
	((int*)(&mVec128))[2] = _z;
	return *this;
}

inline const int IVector3::getZ() const
{
	return ((int*)&mVec128)[2];
}

inline IVector3 & IVector3::setW(int _w)
{
	((int*)(&mVec128))[3] = _w;
	return *this;
}

inline const int IVector3::getW() const
{
	return ((int*)&mVec128)[3];
}

inline IVector3 & IVector3::setElem(int idx, int value)
{
	((int*)(&mVec128))[idx] = value;
	return *this;
}

inline const int IVector3::getElem(int idx) const
{
	return ((int*)&mVec128)[idx];
}

inline IVecIdx IVector3::operator[](int idx)
{
	return IVecIdx(mVec128, idx);
}

inline const int IVector3::operator[](int idx) const
{
	return ((int*)&mVec128)[idx];
}

inline const IVector3 IVector3::operator + (const IVector3 & vec) const
{
	return IVector3(_mm_add_epi32(mVec128, vec.mVec128));
}

inline const IVector3 IVector3::operator - (const IVector3 & vec) const
{
	return IVector3(_mm_sub_epi32(mVec128, vec.mVec128));
}

inline const IVector3 IVector3::operator * (int scalar) const
{
	return IVector3(_mm_mullo_epi32(_mm_set_epi32(scalar, scalar, scalar, scalar), mVec128));
}

inline IVector3 & IVector3::operator += (const IVector3 & vec)
{
	*this = *this + vec;
	return *this;
}

inline IVector3 & IVector3::operator -= (const IVector3 & vec)
{
	*this = *this - vec;
	return *this;
}

inline IVector3 & IVector3::operator *= (int scalar)
{
	*this = *this * scalar;
	return *this;
}

inline const IVector3 IVector3::operator / (int scalar) const
{
	// No sse version exists
	int* vec = (int*)&mVec128;
	return IVector3(vec[0] / scalar, vec[1] / scalar, vec[2] / scalar);
}

inline IVector3 & IVector3::operator /= (int scalar)
{
	*this = *this / scalar;
	return *this;
}

inline const IVector3 IVector3::operator - () const
{
	return IVector3(_mm_sub_epi32(_mm_setzero_si128(), mVec128));
}

inline const IVector3 operator * (int scalar, const IVector3 & vec)
{
	return IVector3(_mm_mullo_epi32(_mm_set_epi32(scalar, scalar, scalar, scalar), vec.get128()));
}

inline const IVector3 mulPerElem(const IVector3 & vec0, const IVector3 & vec1)
{
	return IVector3(_mm_mullo_epi32(vec0.get128(), vec1.get128()));
}

inline const IVector3 divPerElem(const IVector3 & vec0, const IVector3 & vec1)
{
	// No sse version exists
	__m128i v0 = vec0.get128();
	__m128i v1 = vec1.get128();
	int* v0i = (int*)&v0;
	int* v1i = (int*)&v1;
	return IVector3(v0i[0] / v1i[0], v0i[1] / v1i[1], v0i[2] / v1i[2]);
}

inline const IVector3 absPerElem(const IVector3 & vec)
{
	return IVector3(_mm_sign_epi32(vec.get128(), vec.get128()));
}

inline const IVector3 copySignPerElem(const IVector3 & vec0, const IVector3 & vec1)
{
	const __m128i vmask = _mm_set1_epi32(0x7FFFFFFF);
	return IVector3(_mm_or_si128(
		_mm_and_si128(vmask, vec0.get128()),      // Value
		_mm_andnot_si128(vmask, vec1.get128()))); // Signs
}

inline const IVector3 maxPerElem(const IVector3 & vec0, const IVector3 & vec1)
{
	return IVector3(_mm_max_epi32(vec0.get128(), vec1.get128()));
}

inline const int maxElem(const IVector3 & vec)
{
	__m128i s0 = _mm_shuffle_epi32(vec.get128(), _MM_SHUFFLE(0, 0, 0, 0));
	__m128i s1 = _mm_shuffle_epi32(vec.get128(), _MM_SHUFFLE(1, 1, 1, 1));
	__m128i s2 = _mm_shuffle_epi32(vec.get128(), _MM_SHUFFLE(2, 2, 2, 2));
	__m128i res = _mm_max_epi32(_mm_max_epi32(s0, s1), s2);
	return ((int*)&res)[0];
}

inline const IVector3 minPerElem(const IVector3 & vec0, const IVector3 & vec1)
{
	return IVector3(_mm_min_epi32(vec0.get128(), vec1.get128()));
}

inline const int minElem(const IVector3 & vec)
{
	__m128i s0 = _mm_shuffle_epi32(vec.get128(), _MM_SHUFFLE(0, 0, 0, 0));
	__m128i s1 = _mm_shuffle_epi32(vec.get128(), _MM_SHUFFLE(1, 1, 1, 1));
	__m128i s2 = _mm_shuffle_epi32(vec.get128(), _MM_SHUFFLE(2, 2, 2, 2));
	__m128i res = _mm_min_epi32(_mm_min_epi32(s0, s1), s2);
	return ((int*)&res)[0];
}

inline const int sum(const IVector3 & vec)
{
	__m128i s0 = _mm_shuffle_epi32(vec.get128(), _MM_SHUFFLE(0, 0, 0, 0));
	__m128i s1 = _mm_shuffle_epi32(vec.get128(), _MM_SHUFFLE(1, 1, 1, 1));
	__m128i s2 = _mm_shuffle_epi32(vec.get128(), _MM_SHUFFLE(2, 2, 2, 2));
	__m128i res = _mm_add_epi32(_mm_add_epi32(s0, s1), s2);
	return ((int*)&res)[0];
}

#ifdef VECTORMATH_DEBUG

inline void print(const IVector3 & vec)
{
	SSEInt tmp;
	tmp.m128 = vec.get128();
	std::printf("( %i %i %i )\n", tmp.i[0], tmp.i[1], tmp.i[2]);
}

inline void print(const IVector3 & vec, const char * name)
{
	SSEInt tmp;
	tmp.m128 = vec.get128();
	std::printf("%s: ( %i %i %i )\n", name, tmp.i[0], tmp.i[1], tmp.i[2]);
}

#endif // VECTORMATH_DEBUG



// ========================================================
// UVector3
// ========================================================

inline UVector3::UVector3(uint _x, uint _y, uint _z)
{
	mVec128 = _mm_setr_epi32(_x, _y, _z, 0);
}

inline UVector3::UVector3(uint scalar)
{
	mVec128 = _mm_setr_epi32(scalar, scalar, scalar, 0);
}

inline UVector3::UVector3(__m128i vi4)
{
	mVec128 = vi4;
}

inline const UVector3 UVector3::xAxis()
{
	return UVector3(1, 0, 0);
}

inline const UVector3 UVector3::yAxis()
{
	return UVector3(0, 1, 0);
}

inline const UVector3 UVector3::zAxis()
{
	return UVector3(0, 0, 1);
}

inline __m128i UVector3::get128() const
{
	return mVec128;
}

inline UVector3 & UVector3::operator = (const UVector3 & vec)
{
	mVec128 = vec.mVec128;
	return *this;
}

inline UVector3 & UVector3::setX(uint _x)
{
	((uint*)(&mVec128))[0] = _x;
	return *this;
}

inline const uint UVector3::getX() const
{
	return ((uint*)&mVec128)[0];
}

inline UVector3 & UVector3::setY(uint _y)
{
	((uint*)(&mVec128))[1] = _y;
	return *this;
}

inline const uint UVector3::getY() const
{
	return ((uint*)&mVec128)[1];
}

inline UVector3 & UVector3::setZ(uint _z)
{
	((uint*)(&mVec128))[2] = _z;
	return *this;
}

inline const uint UVector3::getZ() const
{
	return ((uint*)&mVec128)[2];
}

inline UVector3 & UVector3::setW(uint _w)
{
	((uint*)(&mVec128))[3] = _w;
	return *this;
}

inline const uint UVector3::getW() const
{
	return ((uint*)&mVec128)[3];
}

inline UVector3 & UVector3::setElem(uint idx, uint value)
{
	((uint*)(&mVec128))[idx] = value;
	return *this;
}

inline const uint UVector3::getElem(uint idx) const
{
	return ((uint*)&mVec128)[idx];
}

inline IVecIdx UVector3::operator[](uint idx)
{
	return IVecIdx(mVec128, idx);
}

inline const uint UVector3::operator[](uint idx) const
{
	return ((uint*)&mVec128)[idx];
}

inline const UVector3 UVector3::operator + (const UVector3 & vec) const
{
	return UVector3(_mm_add_epi32(mVec128, vec.mVec128));
}

inline const UVector3 UVector3::operator - (const UVector3 & vec) const
{
	return UVector3(_mm_sub_epi32(mVec128, vec.mVec128));
}

inline const UVector3 UVector3::operator * (uint scalar) const
{
	return UVector3(_mm_mullo_epi32(_mm_set_epi32(scalar, scalar, scalar, scalar), mVec128));
}

inline UVector3 & UVector3::operator += (const UVector3 & vec)
{
	*this = *this + vec;
	return *this;
}

inline UVector3 & UVector3::operator -= (const UVector3 & vec)
{
	*this = *this - vec;
	return *this;
}

inline UVector3 & UVector3::operator *= (uint scalar)
{
	*this = *this * scalar;
	return *this;
}

inline const UVector3 UVector3::operator / (uint scalar) const
{
	// No sse version exists
	uint* vec = (uint*)&mVec128;
	return UVector3(vec[0] / scalar, vec[1] / scalar, vec[2] / scalar);
}

inline UVector3 & UVector3::operator /= (uint scalar)
{
	*this = *this / scalar;
	return *this;
}

inline const UVector3 operator * (uint scalar, const UVector3 & vec)
{
	return UVector3(_mm_mullo_epi32(_mm_set_epi32(scalar, scalar, scalar, scalar), vec.get128()));
}

inline const UVector3 mulPerElem(const UVector3 & vec0, const UVector3 & vec1)
{
	return UVector3(_mm_mullo_epi32(vec0.get128(), vec1.get128()));
}

inline const UVector3 divPerElem(const UVector3 & vec0, const UVector3 & vec1)
{
	// No sse version exists
	__m128i v0 = vec0.get128();
	__m128i v1 = vec1.get128();
	uint* v0u = (uint*)&v0;
	uint* v1u = (uint*)&v1;
	return UVector3(v0u[0] / v1u[0], v0u[1] / v1u[1], v0u[2] / v1u[2]);
}

inline const UVector3 maxPerElem(const UVector3 & vec0, const UVector3 & vec1)
{
	return UVector3(_mm_max_epu32(vec0.get128(), vec1.get128()));
}

inline const uint maxElem(const UVector3 & vec)
{
	__m128i s0 = _mm_shuffle_epi32(vec.get128(), _MM_SHUFFLE(0, 0, 0, 0));
	__m128i s1 = _mm_shuffle_epi32(vec.get128(), _MM_SHUFFLE(1, 1, 1, 1));
	__m128i s2 = _mm_shuffle_epi32(vec.get128(), _MM_SHUFFLE(2, 2, 2, 2));
	__m128i res = _mm_max_epu32(_mm_max_epu32(s0, s1), s2);
	return ((uint*)&res)[0];
}

inline const UVector3 minPerElem(const UVector3 & vec0, const UVector3 & vec1)
{
	return UVector3(_mm_min_epu32(vec0.get128(), vec1.get128()));
}

inline const uint minElem(const UVector3 & vec)
{
	__m128i s0 = _mm_shuffle_epi32(vec.get128(), _MM_SHUFFLE(0, 0, 0, 0));
	__m128i s1 = _mm_shuffle_epi32(vec.get128(), _MM_SHUFFLE(1, 1, 1, 1));
	__m128i s2 = _mm_shuffle_epi32(vec.get128(), _MM_SHUFFLE(2, 2, 2, 2));
	__m128i res = _mm_min_epu32(_mm_min_epu32(s0, s1), s2);
	return ((uint*)&res)[0];
}

inline const uint sum(const UVector3 & vec)
{
	__m128i s0 = _mm_shuffle_epi32(vec.get128(), _MM_SHUFFLE(0, 0, 0, 0));
	__m128i s1 = _mm_shuffle_epi32(vec.get128(), _MM_SHUFFLE(1, 1, 1, 1));
	__m128i s2 = _mm_shuffle_epi32(vec.get128(), _MM_SHUFFLE(2, 2, 2, 2));
	__m128i res = _mm_add_epi32(_mm_add_epi32(s0, s1), s2);
	return ((uint*)&res)[0];
}

#ifdef VECTORMATH_DEBUG

inline void print(const UVector3 & vec)
{
	SSEUint tmp;
	tmp.m128 = vec.get128();
	std::printf("( %u %u %u )\n", tmp.u[0], tmp.u[1], tmp.u[2]);
}

inline void print(const UVector3 & vec, const char * name)
{
	SSEUint tmp;
	tmp.m128 = vec.get128();
	std::printf("%s: ( %u %u %u )\n", name, tmp.u[0], tmp.u[1], tmp.u[2]);
}

#endif // VECTORMATH_DEBUG



// ========================================================
// IVector4
// ========================================================

inline IVector4::IVector4(int _x, int _y, int _z, int _w)
{
	mVec128 = _mm_setr_epi32(_x, _y, _z, _w);
}

inline IVector4::IVector4(int scalar)
{
	mVec128 = _mm_setr_epi32(scalar, scalar, scalar, scalar);
}

inline IVector4::IVector4(__m128i vi4)
{
	mVec128 = vi4;
}

inline const IVector4 IVector4::xAxis()
{
	return IVector4(1, 0, 0, 0);
}

inline const IVector4 IVector4::yAxis()
{
	return IVector4(0, 1, 0, 0);
}

inline const IVector4 IVector4::zAxis()
{
	return IVector4(0, 0, 1, 0);
}

inline const IVector4 IVector4::wAxis()
{
	return IVector4(0, 0, 0, 1);
}

inline __m128i IVector4::get128() const
{
	return mVec128;
}

inline IVector4 & IVector4::setX(int _x)
{
	((int*)(&mVec128))[0] = _x;
	return *this;
}

inline const int IVector4::getX() const
{
	return ((int*)&mVec128)[0];
}

inline IVector4 & IVector4::setY(int _y)
{
	((int*)(&mVec128))[1] = _y;
	return *this;
}

inline const int IVector4::getY() const
{
	return ((int*)&mVec128)[1];
}

inline IVector4 & IVector4::setZ(int _z)
{
	((int*)(&mVec128))[2] = _z;
	return *this;
}

inline const int IVector4::getZ() const
{
	return ((int*)&mVec128)[2];
}

inline IVector4 & IVector4::setW(int _w)
{
	((int*)(&mVec128))[3] = _w;
	return *this;
}

inline const int IVector4::getW() const
{
	return ((int*)&mVec128)[3];
}

inline IVector4 & IVector4::setElem(int idx, int value)
{
	((int*)(&mVec128))[idx] = value;
	return *this;
}

inline const int IVector4::getElem(int idx) const
{
	return ((int*)&mVec128)[idx];
}

inline IVecIdx IVector4::operator[](int idx)
{
	return IVecIdx(mVec128, idx);
}

inline const int IVector4::operator[](int idx) const
{
	return ((int*)&mVec128)[idx];
}

inline const IVector4 IVector4::operator + (const IVector4 & vec) const
{
	return IVector4(_mm_add_epi32(mVec128, vec.mVec128));
}

inline const IVector4 IVector4::operator - (const IVector4 & vec) const
{
	return IVector4(_mm_sub_epi32(mVec128, vec.mVec128));
}

inline const IVector4 IVector4::operator * (int scalar) const
{
	return IVector4(_mm_mullo_epi32(_mm_set_epi32(scalar, scalar, scalar, scalar), mVec128));
}

inline IVector4 & IVector4::operator += (const IVector4 & vec)
{
	*this = *this + vec;
	return *this;
}

inline IVector4 & IVector4::operator -= (const IVector4 & vec)
{
	*this = *this - vec;
	return *this;
}

inline IVector4 & IVector4::operator *= (int scalar)
{
	*this = *this * scalar;
	return *this;
}

inline const IVector4 IVector4::operator / (int scalar) const
{
	// No sse version exists
	int* vec = (int*)&mVec128;
	return IVector4(vec[0] / scalar, vec[1] / scalar, vec[2] / scalar, vec[3] / scalar);
}

inline IVector4 & IVector4::operator /= (int scalar)
{
	*this = *this / scalar;
	return *this;
}

inline const IVector4 IVector4::operator - () const
{
	return IVector4(_mm_sub_epi32(_mm_setzero_si128(), mVec128));
}

inline const IVector4 operator * (int scalar, const IVector4 & vec)
{
	return IVector4(_mm_mullo_epi32(_mm_set_epi32(scalar, scalar, scalar, scalar), vec.get128()));
}

inline const IVector4 mulPerElem(const IVector4 & vec0, const IVector4 & vec1)
{
	return IVector4(_mm_mullo_epi32(vec0.get128(), vec1.get128()));
}

inline const IVector4 divPerElem(const IVector4 & vec0, const IVector4 & vec1)
{
	// No sse version exists
	__m128i v0 = vec0.get128();
	__m128i v1 = vec1.get128();
	int* v0i = (int*)&v0;
	int* v1i = (int*)&v1;
	return IVector4(v0i[0] / v1i[0], v0i[1] / v1i[1], v0i[2] / v1i[2], v0i[3] / v1i[3]);
}

inline const IVector4 absPerElem(const IVector4 & vec)
{
	return IVector4(_mm_sign_epi32(vec.get128(), vec.get128()));
}

inline const IVector4 copySignPerElem(const IVector4 & vec0, const IVector4 & vec1)
{
	const __m128i vmask = _mm_set1_epi32(0x7FFFFFFF);
	return IVector4(_mm_or_si128(
		_mm_and_si128(vmask, vec0.get128()),      // Value
		_mm_andnot_si128(vmask, vec1.get128()))); // Signs
}

inline const IVector4 maxPerElem(const IVector4 & vec0, const IVector4 & vec1)
{
	return IVector4(_mm_max_epi32(vec0.get128(), vec1.get128()));
}

inline const int maxElem(const IVector4 & vec)
{
	__m128i s0 = _mm_shuffle_epi32(vec.get128(), _MM_SHUFFLE(0, 0, 0, 0));
	__m128i s1 = _mm_shuffle_epi32(vec.get128(), _MM_SHUFFLE(1, 1, 1, 1));
	__m128i s2 = _mm_shuffle_epi32(vec.get128(), _MM_SHUFFLE(2, 2, 2, 2));
	__m128i s3 = _mm_shuffle_epi32(vec.get128(), _MM_SHUFFLE(3, 3, 3, 3));
	__m128i res = _mm_max_epi32(_mm_max_epi32(_mm_max_epi32(s0, s1), s2), s3);
	return ((int*)&res)[0];
}

inline const IVector4 minPerElem(const IVector4 & vec0, const IVector4 & vec1)
{
	return IVector4(_mm_min_epi32(vec0.get128(), vec1.get128()));
}

inline const int minElem(const IVector4 & vec)
{
	__m128i s0 = _mm_shuffle_epi32(vec.get128(), _MM_SHUFFLE(0, 0, 0, 0));
	__m128i s1 = _mm_shuffle_epi32(vec.get128(), _MM_SHUFFLE(1, 1, 1, 1));
	__m128i s2 = _mm_shuffle_epi32(vec.get128(), _MM_SHUFFLE(2, 2, 2, 2));
	__m128i s3 = _mm_shuffle_epi32(vec.get128(), _MM_SHUFFLE(3, 3, 3, 3));
	__m128i res = _mm_min_epi32(_mm_min_epi32(_mm_min_epi32(s0, s1), s2), s3);
	return ((int*)&res)[0];
}

inline const int sum(const IVector4 & vec)
{
	__m128i s0 = _mm_shuffle_epi32(vec.get128(), _MM_SHUFFLE(0, 0, 0, 0));
	__m128i s1 = _mm_shuffle_epi32(vec.get128(), _MM_SHUFFLE(1, 1, 1, 1));
	__m128i s2 = _mm_shuffle_epi32(vec.get128(), _MM_SHUFFLE(2, 2, 2, 2));
	__m128i s3 = _mm_shuffle_epi32(vec.get128(), _MM_SHUFFLE(3, 3, 3, 3));
	__m128i res = _mm_add_epi32(_mm_add_epi32(_mm_add_epi32(s0, s1), s2), s3);
	return ((int*)&res)[0];
}

#ifdef VECTORMATH_DEBUG

inline void print(const IVector4 & vec)
{
	SSEInt tmp;
	tmp.m128 = vec.get128();
	std::printf("( %i %i %i %i )\n", tmp.i[0], tmp.i[1], tmp.i[2], tmp.i[3]);
}

inline void print(const IVector4 & vec, const char * name)
{
	SSEInt tmp;
	tmp.m128 = vec.get128();
	std::printf("%s: ( %i %i %i %i )\n", name, tmp.i[0], tmp.i[1], tmp.i[2], tmp.i[3]);
}

#endif // VECTORMATH_DEBUG

// ========================================================
// UVector4
// ========================================================

inline UVector4::UVector4(uint _x, uint _y, uint _z, uint _w)
{
	mVec128 = _mm_setr_epi32(_x, _y, _z, _w);
}

inline UVector4::UVector4(uint scalar)
{
	mVec128 = _mm_setr_epi32(scalar, scalar, scalar, scalar);
}

inline UVector4::UVector4(__m128i vi4)
{
	mVec128 = vi4;
}

inline const UVector4 UVector4::xAxis()
{
	return UVector4(1, 0, 0, 0);
}

inline const UVector4 UVector4::yAxis()
{
	return UVector4(0, 1, 0, 0);
}

inline const UVector4 UVector4::zAxis()
{
	return UVector4(0, 0, 1, 0);
}

inline const UVector4 UVector4::wAxis()
{
	return UVector4(0, 0, 0, 1);
}

inline __m128i UVector4::get128() const
{
	return mVec128;
}

inline UVector4 & UVector4::operator = (const UVector4 & vec)
{
	mVec128 = vec.mVec128;
	return *this;
}

inline UVector4 & UVector4::setX(uint _x)
{
	((uint*)(&mVec128))[0] = _x;
	return *this;
}

inline const uint UVector4::getX() const
{
	return ((uint*)&mVec128)[0];
}

inline UVector4 & UVector4::setY(uint _y)
{
	((uint*)(&mVec128))[1] = _y;
	return *this;
}

inline const uint UVector4::getY() const
{
	return ((uint*)&mVec128)[1];
}

inline UVector4 & UVector4::setZ(uint _z)
{
	((uint*)(&mVec128))[2] = _z;
	return *this;
}

inline const uint UVector4::getZ() const
{
	return ((uint*)&mVec128)[2];
}

inline UVector4 & UVector4::setW(uint _w)
{
	((uint*)(&mVec128))[3] = _w;
	return *this;
}

inline const uint UVector4::getW() const
{
	return ((uint*)&mVec128)[3];
}

inline UVector4 & UVector4::setElem(uint idx, uint value)
{
	((uint*)(&mVec128))[idx] = value;
	return *this;
}

inline const uint UVector4::getElem(uint idx) const
{
	return ((uint*)&mVec128)[idx];
}

inline IVecIdx UVector4::operator[](uint idx)
{
	return IVecIdx(mVec128, idx);
}

inline const uint UVector4::operator[](uint idx) const
{
	return ((uint*)&mVec128)[idx];
}

inline const UVector4 UVector4::operator + (const UVector4 & vec) const
{
	return UVector4(_mm_add_epi32(mVec128, vec.mVec128));
}

inline const UVector4 UVector4::operator - (const UVector4 & vec) const
{
	return UVector4(_mm_sub_epi32(mVec128, vec.mVec128));
}

inline const UVector4 UVector4::operator * (uint scalar) const
{
	return UVector4(_mm_mullo_epi32(_mm_set_epi32(scalar, scalar, scalar, scalar), mVec128));
}

inline UVector4 & UVector4::operator += (const UVector4 & vec)
{
	*this = *this + vec;
	return *this;
}

inline UVector4 & UVector4::operator -= (const UVector4 & vec)
{
	*this = *this - vec;
	return *this;
}

inline UVector4 & UVector4::operator *= (uint scalar)
{
	*this = *this * scalar;
	return *this;
}

inline const UVector4 UVector4::operator / (uint scalar) const
{
	// No sse version exists
	uint* vec = (uint*)&mVec128;
	return UVector4(vec[0] / scalar, vec[1] / scalar, vec[2] / scalar, vec[3] / scalar);
}

inline UVector4 & UVector4::operator /= (uint scalar)
{
	*this = *this / scalar;
	return *this;
}

inline const UVector4 operator * (uint scalar, const UVector4 & vec)
{
	return UVector4(_mm_mullo_epi32(_mm_set_epi32(scalar, scalar, scalar, scalar), vec.get128()));
}

inline const UVector4 mulPerElem(const UVector4 & vec0, const UVector4 & vec1)
{
	return UVector4(_mm_mullo_epi32(vec0.get128(), vec1.get128()));
}

inline const UVector4 divPerElem(const UVector4 & vec0, const UVector4 & vec1)
{
	// No sse version exists
	__m128i v0 = vec0.get128();
	__m128i v1 = vec1.get128();
	uint* v0u = (uint*)&v0;
	uint* v1u = (uint*)&v1;
	return UVector4(v0u[0] / v1u[0], v0u[1] / v1u[1], v0u[2] / v1u[2], v0u[3] / v1u[3]);
}

inline const UVector4 maxPerElem(const UVector4 & vec0, const UVector4 & vec1)
{
	return UVector4(_mm_max_epu32(vec0.get128(), vec1.get128()));
}

inline const uint maxElem(const UVector4 & vec)
{
	__m128i s0 = _mm_shuffle_epi32(vec.get128(), _MM_SHUFFLE(0, 0, 0, 0));
	__m128i s1 = _mm_shuffle_epi32(vec.get128(), _MM_SHUFFLE(1, 1, 1, 1));
	__m128i s2 = _mm_shuffle_epi32(vec.get128(), _MM_SHUFFLE(2, 2, 2, 2));
	__m128i s3 = _mm_shuffle_epi32(vec.get128(), _MM_SHUFFLE(3, 3, 3, 3));
	__m128i res = _mm_max_epu32(_mm_max_epu32(_mm_max_epu32(s0, s1), s2), s3);
	return ((uint*)&res)[0];
}

inline const UVector4 minPerElem(const UVector4 & vec0, const UVector4 & vec1)
{
	return UVector4(_mm_min_epu32(vec0.get128(), vec1.get128()));
}

inline const uint minElem(const UVector4 & vec)
{
	__m128i s0 = _mm_shuffle_epi32(vec.get128(), _MM_SHUFFLE(0, 0, 0, 0));
	__m128i s1 = _mm_shuffle_epi32(vec.get128(), _MM_SHUFFLE(1, 1, 1, 1));
	__m128i s2 = _mm_shuffle_epi32(vec.get128(), _MM_SHUFFLE(2, 2, 2, 2));
	__m128i s3 = _mm_shuffle_epi32(vec.get128(), _MM_SHUFFLE(3, 3, 3, 3));
	__m128i res = _mm_min_epu32(_mm_min_epu32(_mm_min_epu32(s0, s1), s2), s3);
	return ((uint*)&res)[0];
}

inline const uint sum(const UVector4 & vec)
{
	__m128i s0 = _mm_shuffle_epi32(vec.get128(), _MM_SHUFFLE(0, 0, 0, 0));
	__m128i s1 = _mm_shuffle_epi32(vec.get128(), _MM_SHUFFLE(1, 1, 1, 1));
	__m128i s2 = _mm_shuffle_epi32(vec.get128(), _MM_SHUFFLE(2, 2, 2, 2));
	__m128i s3 = _mm_shuffle_epi32(vec.get128(), _MM_SHUFFLE(3, 3, 3, 3));
	__m128i res = _mm_add_epi32(_mm_add_epi32(_mm_add_epi32(s0, s1), s2), s3);
	return ((uint*)&res)[0];
}

#ifdef VECTORMATH_DEBUG

inline void print(const UVector4 & vec)
{
	SSEUint tmp;
	tmp.m128 = vec.get128();
	std::printf("( %u %u %u %u )\n", tmp.u[0], tmp.u[1], tmp.u[2], tmp.u[3]);
}

inline void print(const UVector4 & vec, const char * name)
{
	SSEUint tmp;
	tmp.m128 = vec.get128();
	std::printf("%s: ( %u %u %u %u )\n", name, tmp.u[0], tmp.u[1], tmp.u[2], tmp.u[3]);
}

#endif // VECTORMATH_DEBUG
//========================================= #TheForgeMathExtensionsEnd ================================================

} // namespace SSE
} // namespace Vectormath

#endif // VECTORMATH_SSE_VECTOR_HPP
