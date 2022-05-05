#include <sha256.h>
#include <cstdio>
#include <cstdlib>
#include <cstring>

namespace Hash
{

static uint32_t rotateRight32(uint32_t n, uint8_t bits)
{
	bits &= 0x1f;
	return (n >> bits) | (n << (32-bits));
}

static uint64_t rotateRight64(uint64_t n, uint8_t bits)
{
	bits &= 0x3f;
	return (n >> bits) | (n << (64-bits));
}

static uint32_t readAsBigEndian32(uint8_t* data)
{
	return 
	(((uint32_t)data[0]) << 24) |
	(((uint32_t)data[1]) << 16) |
	(((uint32_t)data[2]) << 8)  |
	(((uint32_t)data[3]));
}

static uint64_t readAsBigEndian64(uint8_t* data)
{
	return 
	(((uint64_t)data[0]) << 56) |
	(((uint64_t)data[1]) << 48) |
	(((uint64_t)data[2]) << 40) |
	(((uint64_t)data[3]) << 32) |
	(((uint64_t)data[4]) << 24) |
	(((uint64_t)data[5]) << 16) |
	(((uint64_t)data[6]) << 8)  |
	(((uint64_t)data[7]));
}

static void writeAsBigEndian64(uint64_t value, uint8_t* data)
{
	data[0] = value >> 56;
	data[1] = value >> 48;
	data[2] = value >> 40;
	data[3] = value >> 32;
	data[4] = value >> 24;
	data[5] = value >> 16;
	data[6] = value >> 8;
	data[7] = value;
}

/* SHA256 START */

static uint32_t sha256K[64] = 
{
0x428a2f98, 0x71374491, 0xb5c0fbcf, 0xe9b5dba5, 0x3956c25b, 0x59f111f1, 0x923f82a4, 0xab1c5ed5, 
0xd807aa98, 0x12835b01, 0x243185be, 0x550c7dc3, 0x72be5d74, 0x80deb1fe, 0x9bdc06a7, 0xc19bf174, 
0xe49b69c1, 0xefbe4786, 0x0fc19dc6, 0x240ca1cc, 0x2de92c6f, 0x4a7484aa, 0x5cb0a9dc, 0x76f988da, 
0x983e5152, 0xa831c66d, 0xb00327c8, 0xbf597fc7, 0xc6e00bf3, 0xd5a79147, 0x06ca6351, 0x14292967, 
0x27b70a85, 0x2e1b2138, 0x4d2c6dfc, 0x53380d13, 0x650a7354, 0x766a0abb, 0x81c2c92e, 0x92722c85, 
0xa2bfe8a1, 0xa81a664b, 0xc24b8b70, 0xc76c51a3, 0xd192e819, 0xd6990624, 0xf40e3585, 0x106aa070, 
0x19a4c116, 0x1e376c08, 0x2748774c, 0x34b0bcb5, 0x391c0cb3, 0x4ed8aa4a, 0x5b9cca4f, 0x682e6ff3, 
0x748f82ee, 0x78a5636f, 0x84c87814, 0x8cc70208, 0x90befffa, 0xa4506ceb, 0xbef9a3f7, 0xc67178f2
};

/* Init Sha256State */
void sha256Init( Sha256State* state)
{
	state->A = 0x6a09e667;
	state->B = 0xbb67ae85;
	state->C = 0x3c6ef372;
	state->D = 0xa54ff53a;
	state->E = 0x510e527f;
	state->F = 0x9b05688c;
	state->G = 0x1f83d9ab;
	state->H = 0x5be0cd19;
}


/* Calculate a group data */
void sha256Count(Sha256State* state, const void* data)
{
	uint32_t DATA[64];
	
	size_t i = 0;
	for(; i < 16; i++)
	{
		DATA[i] = readAsBigEndian32(((uint8_t*)data) + i*4);//changeEndian32(((uint32_t*)data)[i]);
	}
	
	for(; i < 64; i++)
	{
		uint32_t S0 = rotateRight32(DATA[i-15],7) ^ rotateRight32(DATA[i-15],18) ^ (DATA[i-15] >> 3);
		uint32_t S1 = rotateRight32(DATA[i-2],17) ^ rotateRight32(DATA[i-2],19) ^ (DATA[i-2] >> 10);
		DATA[i] = DATA[i-16] + S0 + DATA[i-7] + S1;
	}
	
	uint32_t a = state->A;
	uint32_t b = state->B;
	uint32_t c = state->C;
	uint32_t d = state->D;
	uint32_t e = state->E;
	uint32_t f = state->F;
	uint32_t g = state->G;
	uint32_t h = state->H;
	
	for(i = 0; i < 64; i++)
	{
		uint32_t T0 = rotateRight32(e,6) ^ rotateRight32(e,11) ^ rotateRight32(e,25);
		uint32_t T1 = (e & f) ^ ((~e) & g);
		uint32_t T2 = h + T0 + T1 + sha256K[i] + DATA[i];
		uint32_t T3 = rotateRight32(a,2) ^ rotateRight32(a,13) ^ rotateRight32(a,22);
		uint32_t T4 = (a & b) ^ (a & c) ^ (b & c);
		uint32_t T5 = T3 + T4;
		
		h = g;
		g = f;
		f = e;
		e = d + T2;
		d = c;
		c = b;
		b = a;
		a = T2 + T5;
	}
	
	state->A += a;
	state->B += b;
	state->C += c;
	state->D += d;
	state->E += e;
	state->F += f;
	state->G += g;
	state->H += h;
}


/* Calculate the last group data */
void sha256Tail(Sha256State* state,void* data, uint8_t currentBytes, uint64_t totalBytes)
{
	/* fill current group data by 1000... */
	if(currentBytes != 56)
	{
		((uint8_t*)data)[currentBytes] = 0x80;
		for(uint8_t i = currentBytes + 1; i < 64; i++)
		{
			((uint8_t*)data)[i] = 0;
		}
	}
	
	/* need one more group */
	if(currentBytes > 56)
	{
		sha256Count(state, data);
		uint8_t oneMore[64] = {0};
		writeAsBigEndian64(totalBytes * 8, oneMore+56);
		sha256Count(state, oneMore);
	}
	/* needn't one more group */
	else
	{
		writeAsBigEndian64(totalBytes * 8, ((uint8_t*)data+56));
		sha256Count(state, data);
	}
}


/* Get the SHA256 value as hex string */
void sha256Result(Sha256State* state, char* result)
{
sprintf(result, "%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x",
			(uint8_t)(state->A>>24),
			(uint8_t)(state->A>>16),
			(uint8_t)(state->A>>8),
			(uint8_t)(state->A),
			
			(uint8_t)(state->B>>24),
			(uint8_t)(state->B>>16),
			(uint8_t)(state->B>>8),
			(uint8_t)(state->B),
			
			(uint8_t)(state->C>>24),
			(uint8_t)(state->C>>16),
			(uint8_t)(state->C>>8),
			(uint8_t)(state->C),
			
			(uint8_t)(state->D>>24),
			(uint8_t)(state->D>>16),
			(uint8_t)(state->D>>8),
			(uint8_t)(state->D),
			
			(uint8_t)(state->E>>24),
			(uint8_t)(state->E>>16),
			(uint8_t)(state->E>>8),
			(uint8_t)(state->E),
			
			(uint8_t)(state->F>>24),
			(uint8_t)(state->F>>16),
			(uint8_t)(state->F>>8),
			(uint8_t)(state->F),
			
			(uint8_t)(state->G>>24),
			(uint8_t)(state->G>>16),
			(uint8_t)(state->G>>8),
			(uint8_t)(state->G),
			
			(uint8_t)(state->H>>24),
			(uint8_t)(state->H>>16),
			(uint8_t)(state->H>>8),
			(uint8_t)(state->H));
}

/* Reset Sha256 */
void sha256Reset(Sha256* sha256)
{
	sha256Init(&(sha256->state));
	sha256->length = 0;
	sha256->used = 0;
	sha256->hex[0] = 0;
}

/* Add data into Sha256State autoly */
void sha256Update(Sha256* sha256, const void* data, size_t length)
{
    const uint8_t* pdata = reinterpret_cast<const uint8_t*>(data);
	/* Forgot reset */
	if(sha256->hex[0] != 0)
	{
		sha256Reset(sha256);
	}

	size_t need = 64 - sha256->used;
	while(length >= need)
	{
		void* p = sha256->buffer + sha256->used;
		memcpy(p, pdata, need);
		sha256Count(&(sha256->state), sha256->buffer);
		sha256->length += 64;
		sha256->used = 0;

		pdata += need;
		length -= need;
		need = 64;
	}

	void* p = sha256->buffer + sha256->used;
	memcpy(p, pdata, length);
	sha256->used += length;
}


/* Get the hex */
const char* sha256Hex(Sha256* sha256)
{
	/* Invoke repeatedly */
	if(sha256->hex[0] != 0)
	{
		return sha256->hex;
	}

	sha256->length += sha256->used;
	sha256Tail(&(sha256->state), sha256->buffer, sha256->used, sha256->length);
	sha256Result(&(sha256->state), sha256->hex);
	return (const char*)(sha256->hex);
}

const char* sha256OfData(Sha256* sha256, const void* data, size_t length)
{
	sha256Reset(sha256);
	sha256Update(sha256, data, length);
	return sha256Hex(sha256);
}

const char* sha256OfString(Sha256* sha256, const char* str)
{
	return sha256OfData(sha256, (const void*)(str), strlen(str));
}

} // namespace Hash