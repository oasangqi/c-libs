﻿#pragma once 

#include "EVComm.h"

namespace evwork
{

	class CWriter
		: public IWriter
	{
	public:
		CWriter();
		virtual ~CWriter();

		virtual int send(const std::string& strIp, uint16_t uPort, const char* pData, size_t uSize);
	};

}
