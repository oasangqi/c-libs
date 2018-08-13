#pragma once 

#include "FormDef.h"

#include <tr1/unordered_map>

namespace evwork
{

	class CJsonMFC
		: public IAppContext
	{
	public:
		CJsonMFC();
		virtual ~CJsonMFC();

		virtual void addEntry(FormEntry* pEntry, void* pTarget);

		virtual void RequestDispatch(Jpacket& packet, IConn* pConn);

	protected:
		virtual void DefaultDispatch(Jpacket& packet, IConn* pConn);

	protected:
		typedef std::tr1::unordered_map<uint32_t, FormEntry*> ENTRY_MAP_t;

		ENTRY_MAP_t m_mapEntry;
	};

}
