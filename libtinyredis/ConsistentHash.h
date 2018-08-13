#pragma once 

#include <stdint.h>
#include <string.h>

#include <iostream>

class CHashFunction
{
public:
	static uint32_t fnvHash(const char *pData, size_t uSize)
	{
		register uint32_t uMagic = 16777619;
		register uint32_t uHash = 0x811C9DC5;//2166136261L;

		for (size_t i = 0; i < uSize; ++i)
		{
			uHash = (uHash ^ (*(unsigned char *)(pData+i))) * uMagic;
		}

		uHash += uHash << 13;
		uHash ^= uHash >> 7;
		uHash += uHash << 3;
		uHash ^= uHash >> 17;
		uHash += uHash << 5;

		return uHash;    
	}

	static uint32_t fnvHash(uint32_t u32)
	{
		return fnvHash((const char *) &u32, sizeof(u32));
	}
};
