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
#pragma once
#include "vectormath/vectormath_settings.hpp"


namespace Vectormath
{
VECTORMATH_INLINE_SSE_NAMESPACE namespace SSE
{

class FloatInVec;
class DoubleInVec;
typedef __m128i Vector4Int;

#include "internal.hpp"
// ========================================================
// BoolInVec
// ========================================================

// Vectorized boolean value.
VECTORMATH_ALIGNED_TYPE_PRE class BoolInVec
{
    __m128 mData;

	inline BoolInVec(__m128 vec);
	inline BoolInVec(DSSEVec4 vec);
	
public:
    
    inline BoolInVec() { } //-V730

    // matches standard type conversions
    //
    inline BoolInVec(const FloatInVec & vec);

	inline BoolInVec(const DoubleInVec & vec);

    // explicit cast from bool
    //
    explicit inline BoolInVec(bool scalar);

#ifdef VECTORMATH_NO_SCALAR_CAST
    // explicit cast to bool
    inline bool getAsBool() const;
#else // !VECTORMATH_NO_SCALAR_CAST
    // implicit cast to bool
    inline operator bool() const;
#endif // VECTORMATH_NO_SCALAR_CAST

    // get vector data
    // bool value is splatted across all word slots of vector as 0 (false) or -1 (true)
    //
    inline __m128 get128() const;

    // returns sing bits of all components in 0:3 bits positions. 4:31 bits are 0.
    inline uint32_t getFlags() const;

    // operators
    //
    inline const BoolInVec operator ! () const;
    inline BoolInVec & operator =  (const BoolInVec & vec);
    inline BoolInVec & operator &= (const BoolInVec & vec);
    inline BoolInVec & operator ^= (const BoolInVec & vec);
    inline BoolInVec & operator |= (const BoolInVec & vec);

    // friend functions
    //
    friend inline const BoolInVec operator == (const BoolInVec  & vec0, const BoolInVec  & vec1);
    friend inline const BoolInVec operator != (const BoolInVec  & vec0, const BoolInVec  & vec1);
    friend inline const BoolInVec operator &  (const BoolInVec  & vec0, const BoolInVec  & vec1);
    friend inline const Vector4Int operator & (const BoolInVec  & vec0, const Vector4Int & vec1);
    friend inline const BoolInVec operator ^  (const BoolInVec  & vec0, const BoolInVec  & vec1);
    friend inline const BoolInVec operator |  (const BoolInVec  & vec0, const BoolInVec  & vec1);
    friend inline const BoolInVec select(const BoolInVec & vec0, const BoolInVec & vec1, const BoolInVec & select_vec1);

	friend inline const BoolInVec operator <  (const FloatInVec & vec0, const FloatInVec & vec1);
	friend inline const BoolInVec operator <= (const FloatInVec & vec0, const FloatInVec & vec1);
	friend inline const BoolInVec operator >  (const FloatInVec & vec0, const FloatInVec & vec1);
	friend inline const BoolInVec operator >= (const FloatInVec & vec0, const FloatInVec & vec1);
	friend inline const BoolInVec operator == (const FloatInVec & vec0, const FloatInVec & vec1);
	friend inline const BoolInVec operator != (const FloatInVec & vec0, const FloatInVec & vec1);

	friend inline const BoolInVec operator <  (const DoubleInVec & vec0, const DoubleInVec & vec1);
	friend inline const BoolInVec operator <= (const DoubleInVec & vec0, const DoubleInVec & vec1);
	friend inline const BoolInVec operator >  (const DoubleInVec & vec0, const DoubleInVec & vec1);
	friend inline const BoolInVec operator >= (const DoubleInVec & vec0, const DoubleInVec & vec1);
	friend inline const BoolInVec operator == (const DoubleInVec & vec0, const DoubleInVec & vec1);
	friend inline const BoolInVec operator != (const DoubleInVec & vec0, const DoubleInVec & vec1);

} VECTORMATH_ALIGNED_TYPE_POST;

// ========================================================
// BoolInVec functions
// ========================================================

// operators
//
inline const BoolInVec operator == (const BoolInVec & vec0, const BoolInVec & vec1);
inline const BoolInVec operator != (const BoolInVec & vec0, const BoolInVec & vec1);
inline const BoolInVec operator &  (const BoolInVec & vec0, const BoolInVec & vec1);
inline const Vector4Int operator & (const BoolInVec & vec0, const Vector4Int& vec1);
inline const BoolInVec operator ^  (const BoolInVec & vec0, const BoolInVec & vec1);
inline const BoolInVec operator |  (const BoolInVec & vec0, const BoolInVec & vec1);

// select between vec0 and vec1 using BoolInVec.
// false selects vec0, true selects vec1
//
inline const BoolInVec select(const BoolInVec & vec0, const BoolInVec & vec1, const BoolInVec & select_vec1);

} // namespace SSE
} // namespace Vectormath

// ========================================================
// BoolInVec implementation
// ========================================================

#include <emmintrin.h>
#include "floatinvec.hpp"
#include "doubleinvec.hpp"

namespace Vectormath
{
namespace SSE
{

inline BoolInVec::BoolInVec(__m128 vec)
{
    mData = vec;
}

inline BoolInVec::BoolInVec(DSSEVec4 vec)
{
	int x = (int)vec.l[0];
	int y = (int)vec.l[1];
	int z = (int)vec.l[2];
	int w = (int)vec.l[3];
	mData = _mm_castsi128_ps(_mm_setr_epi32(x, y, z, w));
}

inline BoolInVec::BoolInVec(const FloatInVec & vec)
{
    *this = (vec != FloatInVec(0.0f));
}

inline BoolInVec::BoolInVec(const DoubleInVec & vec)
{
	*this = (vec != DoubleInVec(0.0f));
}

inline BoolInVec::BoolInVec(bool scalar)
{
    union
    {
        unsigned int mask;
        float f;
    } tmp;

    tmp.mask = -(int)scalar;
    mData = _mm_set1_ps(tmp.f);
}

#ifdef VECTORMATH_NO_SCALAR_CAST
inline bool BoolInVec::getAsBool() const
#else
inline BoolInVec::operator bool() const
#endif
{
	// cast to unsigned char* instead of bool*
	// mData of type __m128 stores either 0xffffffff or 0x0 to reflect a boolean value
	// 0xffffffff as seen as a float is NaN, which would cause the cast to bool* to trigger an
	// undefined behavior error because it assumes NaN to be uninitialized.

    // Redone: if all sign bit is not 0 (__m128 stores either 0xffffffff or 0x0 to reflect a boolean value), then BoolInVec is true
    return _mm_movemask_ps(mData) == 0xf;
}

inline __m128 BoolInVec::get128() const
{
    return mData;
}

inline uint32_t BoolInVec::getFlags() const
{
    return uint32_t(_mm_movemask_ps(mData));
}

inline const BoolInVec BoolInVec::operator ! () const
{
    return BoolInVec(_mm_andnot_ps(mData, _mm_cmpneq_ps(_mm_setzero_ps(), _mm_setzero_ps())));
}

inline BoolInVec & BoolInVec::operator = (const BoolInVec & vec)
{
    mData = vec.mData;
    return *this;
}

inline BoolInVec & BoolInVec::operator &= (const BoolInVec & vec)
{
    *this = *this & vec;
    return *this;
}

inline BoolInVec & BoolInVec::operator ^= (const BoolInVec & vec)
{
    *this = *this ^ vec;
    return *this;
}

inline BoolInVec & BoolInVec::operator |= (const BoolInVec & vec)
{
    *this = *this | vec;
    return *this;
}

inline const BoolInVec operator == (const BoolInVec & vec0, const BoolInVec & vec1)
{
    return BoolInVec(_mm_cmpeq_ps(vec0.get128(), vec1.get128()));
}

inline const BoolInVec operator != (const BoolInVec & vec0, const BoolInVec & vec1)
{
    return BoolInVec(_mm_cmpneq_ps(vec0.get128(), vec1.get128()));
}

inline const BoolInVec operator & (const BoolInVec & vec0, const BoolInVec & vec1)
{
    return BoolInVec(_mm_and_ps(vec0.get128(), vec1.get128()));
}

inline const Vector4Int operator & (const BoolInVec& vec0, const Vector4Int& vec1)
{
    return _mm_and_si128(_mm_castps_si128(vec0.get128()), vec1);
}

inline const BoolInVec operator | (const BoolInVec & vec0, const BoolInVec & vec1)
{
    return BoolInVec(_mm_or_ps(vec0.get128(), vec1.get128()));
}

inline const BoolInVec operator ^ (const BoolInVec & vec0, const BoolInVec & vec1)
{
    return BoolInVec(_mm_xor_ps(vec0.get128(), vec1.get128()));
}

inline const BoolInVec select(const BoolInVec & vec0, const BoolInVec & vec1, const BoolInVec & select_vec1)
{
    return BoolInVec(sseSelect(vec0.get128(), vec1.get128(), select_vec1.get128()));
}

} // namespace SSE
} // namespace Vectormath


