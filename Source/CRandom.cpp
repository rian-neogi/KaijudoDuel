#include "CRandom.h"

#include <atomic>
#include <chrono>
#include <cstdint>

CRandom::CRandom(void)
{
   rseed = 1;
   mti=CMATH_N+1;
}

// Returns a number from 0 to n (excluding n)
unsigned int CRandom::Random( unsigned int n )
{
    unsigned long y;
    static unsigned long mag01[2]={0x0, CMATH_MATRIX_A};

   if(n==0)
     return(0);

    /* mag01[x] = x * MATRIX_A for x=0,1 */

    if (mti >= CMATH_N) { /* generate N words at one time */
        int kk;

        if (mti == CMATH_N+1)   /* if sgenrand() has not been called, */
            SetRandomSeed(4357); /* a default initial seed is used   */

        for (kk=0;kk<CMATH_N-CMATH_M;kk++) {
            y = (mt[kk]&CMATH_UPPER_MASK)|(mt[kk+1]&CMATH_LOWER_MASK);
            mt[kk] = mt[kk+CMATH_M] ^ (y >> 1) ^ mag01[y & 0x1];
        }
        for (;kk<CMATH_N-1;kk++) {
            y = (mt[kk]&CMATH_UPPER_MASK)|(mt[kk+1]&CMATH_LOWER_MASK);
            mt[kk] = mt[kk+(CMATH_M-CMATH_N)] ^ (y >> 1) ^ mag01[y & 0x1];
        }
        y = (mt[CMATH_N-1]&CMATH_UPPER_MASK)|(mt[0]&CMATH_LOWER_MASK);
        mt[CMATH_N-1] = mt[CMATH_M-1] ^ (y >> 1) ^ mag01[y & 0x1];

        mti = 0;
    }

    y = mt[mti++];
    y ^= CMATH_TEMPERING_SHIFT_U(y);
    y ^= CMATH_TEMPERING_SHIFT_S(y) & CMATH_TEMPERING_MASK_B;
    y ^= CMATH_TEMPERING_SHIFT_T(y) & CMATH_TEMPERING_MASK_C;
    y ^= CMATH_TEMPERING_SHIFT_L(y);

    return (y%n);

}

void CRandom::SetRandomSeed(unsigned int n)
{
   /* setting initial seeds to mt[N] using         */
   /* the generator Line 25 of Table 1 in          */
   /* [KNUTH 1981, The Art of Computer Programming */
   /*    Vol. 2 (2nd Ed.), pp102]                  */
   mt[0]= n & 0xffffffff;
   for (mti=1; mti<CMATH_N; mti++)
     mt[mti] = (69069 * mt[mti-1]) & 0xffffffff;

   rseed = n;
}
unsigned int CRandom::GetRandomSeed(void)
{
   return(rseed);
}

void CRandom::Randomize(void)
{
	SetRandomSeed(GenerateRandomSeed());
}

unsigned int CRandom::GenerateRandomSeed(void)
{
	static std::atomic<unsigned long long> sequence(0);
	std::uint64_t value = static_cast<std::uint64_t>(
		std::chrono::high_resolution_clock::now().time_since_epoch().count());
	value ^= (sequence.fetch_add(1, std::memory_order_relaxed) + 1) *
		UINT64_C(0x9e3779b97f4a7c15);
	value ^= value >> 30;
	value *= UINT64_C(0xbf58476d1ce4e5b9);
	value ^= value >> 27;
	value *= UINT64_C(0x94d049bb133111eb);
	value ^= value >> 31;
	unsigned int seed = static_cast<unsigned int>(value ^ (value >> 32));
	return seed == 0 ? 1U : seed;
}
