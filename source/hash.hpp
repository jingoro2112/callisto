#ifndef HASH_H
#define HASH_H
/* ------------------------------------------------------------------------- */

// murmer hash

//------------------------------------------------------------------------------
inline unsigned int hash( const void *dat, const unsigned int len )
{
	static const unsigned int c1 = 0xCC9E2D51;
	static const unsigned int c2 = 0x1B873593;
	static const unsigned int r1 = 15;
	static const unsigned int r2 = 13;
	static const unsigned int m = 5;
	static const unsigned int n = 0xE6546B64;

	unsigned int hash = 0x811C9DC5;

	const int nblocks = len / 4;
	const unsigned int *blocks = (const unsigned int *)dat;
	int i;
	for (i = 0; i < nblocks; i++)
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

//------------------------------------------------------------------------------
inline unsigned int hashStr( const void *dat )
{
	static const unsigned int c1 = 0xCC9E2D51;
	static const unsigned int c2 = 0x1B873593;
	static const unsigned int r1 = 15;
	static const unsigned int r2 = 13;
	static const unsigned int m = 5;
	static const unsigned int n = 0xE6546B64;

	unsigned int hash = 0x811C9DC5;

	//const unsigned int *blocks = (const unsigned int *)dat;
	const char* data = (const char *)dat;

	while( data[0] && data[1] && data[2] && data[3] )
	{
		unsigned int k = *(unsigned int *)data;

		data += 4;
		k *= c1;
		k = (k << r1) | (k >> (32 - r1));
		k *= c2;

		hash ^= k;
		hash = ((hash << r2) | (hash >> (32 - r2))) * m + n;
	}

	unsigned int k1 = 0;
	unsigned int len = (unsigned int)((const char *)data - (const char *)dat);

	// must check in this order so as not to access memory past the end
	// of the string, even for read
	if ( !data[0] )
	{
		goto noAvalanche;
	}
	else if ( !data[1] )
	{
		len++;
		goto avalanche1;
	}
	else if ( !data[2] )
	{
		len += 2;
	}
	else
	{
		len += 3;
		k1 ^= data[2] << 16;
	}

	k1 ^= data[1] << 8;

avalanche1:

	k1 ^= data[0];
	k1 *= c1;
	k1 = (k1 << r1) | (k1 >> (32 - r1));
	k1 *= c2;
	hash ^= k1;

noAvalanche:

	hash ^= len;
	hash ^= (hash >> 16);
	hash *= 0x85ebca6b;
	hash ^= (hash >> 13);
	hash *= 0xc2b2ae35;
	hash ^= (hash >> 16);

	return hash;
}

#endif
