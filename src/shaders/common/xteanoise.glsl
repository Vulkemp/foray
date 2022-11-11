#ifndef XTEANOISE_GLSL
#define XTEANOISE_GLSL

// XTEA algorithm (public domain) https://www.cix.co.uk/~klockstone/xtea.pdf

uint CalculateSeedXTEA(uint seed0, uint seed1)
{
	const uint num_rounds = 16;
	const uint key[4] = {0x44DFD203, 0x7D5C4117, 0xAAC42FA6, 0x6F94B357};
	uint sum = 0;
	uint delta = 0x9E3779B9;
	for (uint i = 0; i < num_rounds; i++)
	{
        seed0 += (((seed1 << 4) ^ (seed1 >> 5)) + seed1) ^ (sum + key[sum & 3]);
        sum += delta;
        seed1 += (((seed0 << 4) ^ (seed0 >> 5)) + seed0) ^ (sum + key[(sum>>11) & 3]);
    }
    return seed0 + seed1;
}

uint CalculateSeedXTEA(ivec2 texelPos, uint seed)
{
    return CalculateSeedXTEA((texelPos.x << 16) | texelPos.y, seed);
}

#endif // XTEANOISE_GLSL
