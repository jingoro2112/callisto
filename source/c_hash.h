#ifndef CALLISTO_HASH_H
#define CALLISTO_HASH_H
/* ------------------------------------------------------------------------- */

#include "arch.h"
#include <string.h>

// murmur

namespace Callisto
{

//------------------------------------------------------------------------------
inline unsigned int hash32( const void *dat, const size_t len )
{
	static const unsigned int c1 = 0xCC9E2D51;
	static const unsigned int c2 = 0x1B873593;
	static const unsigned int r1 = 15;
	static const unsigned int r2 = 13;
	static const unsigned int m = 5;
	static const unsigned int n = 0xE6546B64;

	unsigned int hash = 0x811C9DC5;

	const size_t nblocks = len / 4;
	const unsigned int *blocks = (const unsigned int *)dat;
	for (size_t i = 0; i < nblocks; i++)
	{
		unsigned int k = blocks[i];
		k *= c1;
		k = (k << r1) | (k >> (32 - r1));
		k *= c2;

		hash ^= k;
		hash = ((hash << r2) | (hash >> (32 - r2))) * m + n;
	}

	const unsigned char *tail = (const unsigned char *)((const unsigned char*)dat + nblocks * 4);
	unsigned int k1 = 0;

	switch (len & 3)
	{
		case 3:
			k1 ^= tail[2] << 16;
		case 2:
			k1 ^= tail[1] << 8;
		case 1:
			k1 ^= tail[0];

			k1 *= c1;
			k1 = (k1 << r1) | (k1 >> (32 - r1));
			k1 *= c2;
			hash ^= k1;
	}

	hash ^= len;
	hash ^= (hash >> 16);
	hash *= 0x85EBCA6B;
	hash ^= (hash >> 13);
	hash *= 0xC2B2AE35;
	hash ^= (hash >> 16);

	return hash;
}

// metro

//------------------------------------------------------------------------------
inline uint64_t rotate_right(uint64_t v, unsigned k)
{
	return (v >> k) | (v << (64 - k));
}

//------------------------------------------------------------------------------
inline uint64_t hash64( const void* buffer, const uint64_t length )
{
	const uint64_t k0 = 0xD6D018F5;
	const uint64_t k1 = 0xA2AA033B;
	const uint64_t k2 = 0x62992FC1;
	const uint64_t k3 = 0x30BC5B29;

	const uint8_t* ptr = reinterpret_cast<const uint8_t*>(buffer);
	const uint8_t* const end = ptr + length;

	uint64_t h = k2 * k0;

	if (length >= 32)
	{
		uint64_t v[4];
		v[0] = h;
		v[1] = h;
		v[2] = h;
		v[3] = h;

		do
		{
			v[0] += static_cast<uint64_t>(*reinterpret_cast<const uint64_t*>(ptr)) * k0; ptr += 8; v[0] = rotate_right(v[0],29) + v[2];
			v[1] += static_cast<uint64_t>(*reinterpret_cast<const uint64_t*>(ptr)) * k1; ptr += 8; v[1] = rotate_right(v[1],29) + v[3];
			v[2] += static_cast<uint64_t>(*reinterpret_cast<const uint64_t*>(ptr)) * k2; ptr += 8; v[2] = rotate_right(v[2],29) + v[0];
			v[3] += static_cast<uint64_t>(*reinterpret_cast<const uint64_t*>(ptr)) * k3; ptr += 8; v[3] = rotate_right(v[3],29) + v[1];
		}
		while (ptr <= (end - 32));

		v[2] ^= rotate_right(((v[0] + v[3]) * k0) + v[1], 37) * k1;
		v[3] ^= rotate_right(((v[1] + v[2]) * k1) + v[0], 37) * k0;
		v[0] ^= rotate_right(((v[0] + v[2]) * k0) + v[3], 37) * k1;
		v[1] ^= rotate_right(((v[1] + v[3]) * k1) + v[2], 37) * k0;
		h += v[0] ^ v[1];
	}

	if ((end - ptr) >= 16)
	{
		uint64_t v0 = h + (static_cast<uint64_t>(*reinterpret_cast<const uint64_t*>(ptr)) * k2); ptr += 8; v0 = rotate_right(v0,29) * k3;
		uint64_t v1 = h + (static_cast<uint64_t>(*reinterpret_cast<const uint64_t*>(ptr)) * k2); ptr += 8; v1 = rotate_right(v1,29) * k3;
		v0 ^= rotate_right(v0 * k0, 21) + v1;
		v1 ^= rotate_right(v1 * k3, 21) + v0;
		h += v1;
	}

	if ((end - ptr) >= 8)
	{
		h += static_cast<uint64_t>(*reinterpret_cast<const uint64_t*>(ptr)) * k3; ptr += 8;
		h ^= rotate_right(h, 55) * k1;
	}

	if ((end - ptr) >= 4)
	{
		h += static_cast<uint64_t>(*reinterpret_cast<const uint32_t*>(ptr)) * k3; ptr += 4;
		h ^= rotate_right(h, 26) * k1;
	}

	if ((end - ptr) >= 2)
	{
		h += static_cast<uint64_t>(*reinterpret_cast<const uint16_t*>(ptr)) * k3; ptr += 2;
		h ^= rotate_right(h, 48) * k1;
	}

	if ((end - ptr) >= 1)
	{
		h += static_cast<uint64_t>(*reinterpret_cast<const uint8_t*>(ptr)) * k3;
		h ^= rotate_right(h, 37) * k1;
	}

	h ^= rotate_right(h, 28);
	h *= k0;
	h ^= rotate_right(h, 29);

	return h;
}

}

#endif
