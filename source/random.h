#ifndef RANDOM_H
#define RANDOM_H
/*----------------------------------------------------------------------------*/

#include <time.h>

//------------------------------------------------------------------------------
class CRandom
{
public:

	int fromToInclusive( const int from, const int to ) { return (from >= to) ? from : (kiss() % ((to - from) + 1)) + from; }

	inline unsigned int kiss(); // very fast and very very good
	inline unsigned long long mtwister(); // mersenne twister, about as kickass as it gets, and only a smidge slower than kiss
	inline bool binary(); // very fast binary randomness

	inline CRandom( const unsigned int seed =0 );

private:

	unsigned int m_binarySeed;

	static const int CRANDOM_NN = 312;
	static const int CRANDOM_MM = 156;
	unsigned long long m_mt[CRANDOM_NN];
	unsigned long long m_mag01[2];
	int m_mti;

	unsigned int kissx;
	unsigned int kissy;
	unsigned int kissz;
	unsigned int kissc;
};

//------------------------------------------------------------------------------
unsigned long long CRandom::mtwister()
{
	const unsigned long long UM = 0xFFFFFFFF80000000ULL;
	const unsigned long long LM = 0x7FFFFFFFULL;
	
	unsigned long long x;

	if ( m_mti >= CRANDOM_NN )
	{
		int i;

		for (i=0; i<(CRANDOM_NN - CRANDOM_MM); i++ )
		{
			x = (m_mt[i] & UM) | (m_mt[i+1] & LM);
			m_mt[i] = m_mt[i+CRANDOM_MM] ^ (x>>1) ^ m_mag01[(int)(x&1ULL)];
		}

		for (; i<CRANDOM_NN-1; i++)
		{
			x = (m_mt[i] & UM)|(m_mt[i+1] & LM);
			m_mt[i] = m_mt[i+(CRANDOM_MM-CRANDOM_NN)] ^ (x>>1) ^ m_mag01[(int)(x&1ULL)];
		}
		
		x = (m_mt[CRANDOM_NN-1] & UM) | (m_mt[0] & LM);
		m_mt[CRANDOM_NN-1] = m_mt[CRANDOM_MM-1] ^ (x>>1) ^ m_mag01[(int)(x&1ULL)];

		m_mti = 0;
	}

	x = m_mt[m_mti++];

	x ^= (x >> 29) & 0x5555555555555555ULL;
	x ^= (x << 17) & 0x71D67FFFEDA60000ULL;
	x ^= (x << 37) & 0xFFF7EEE000000000ULL;
	x ^= (x >> 43);

	return x;
}

//------------------------------------------------------------------------------
unsigned int CRandom::kiss()
{
	unsigned long long a = 698769069ULL;
	
	kissx = 69069*kissx + 12345;
	kissy ^= (kissy<<13);
	kissy ^= (kissy>>17);
	kissy ^= (kissy<<5);
	
	unsigned long long t = a*kissz + kissc;
	
	kissc = (t>>32);
	kissz = (unsigned int)t;
	
	return kissx + kissy + kissz;
}

//------------------------------------------------------------------------------
bool CRandom::binary()
{
	if ( m_binarySeed & 0x20000 )
	{
		m_binarySeed = ((m_binarySeed ^ 19) << 1) | 1;
		return true;
	}
	else
	{
		m_binarySeed <<= 1;
		return false;
	}
}

//------------------------------------------------------------------------------
CRandom::CRandom( unsigned int seed )
{
	m_binarySeed = seed ? seed  : ((unsigned int)time(0) & 0x7FFFFFFF); // force positive

	// turn the crank on a single minimal rand standard to init the mercene constant
	const int k = m_binarySeed / 127773;
	const int s2 = 16807 * (m_binarySeed - k * 127773) - 2836 * k;
	const unsigned long long R = (s2 < 0) ? (s2 + 2147483647) : s2;

	m_mti = CRANDOM_NN + 1; 
	m_mt[0] = m_binarySeed | (R << 32);

	for ( m_mti = 1; m_mti<CRANDOM_NN; m_mti++ )
	{
		m_mt[m_mti] = (6364136223846793005ULL * (m_mt[m_mti-1] ^ (m_mt[m_mti-1] >> 62)) + m_mti);
	}
	m_mag01[0] = 0;
	m_mag01[1] = 0xB5026F5AA96619E9ULL;

	// cinch to init these now
	kissx = (unsigned int)mtwister();
	kissy = (unsigned int)mtwister();
	kissz = (unsigned int)mtwister();
	kissc = (unsigned int)mtwister();
}

#endif
