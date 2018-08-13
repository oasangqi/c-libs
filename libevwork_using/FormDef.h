#pragma once 

#include "EVComm.h"
#include "JsonPacket.h"

#include <memory>

namespace evwork
{

	class IAppContext
	{
	public:
		virtual ~IAppContext() {};

		virtual void RequestDispatch(Jpacket& packet, IConn* pConn) = 0;
	};
	class IAppContextAware
	{
	protected:
		IAppContext* m_pAppContext;
	public:
		IAppContextAware() : m_pAppContext(NULL) {}
		virtual ~IAppContextAware() {}

		void setAppContext(IAppContext* pAppContext) { m_pAppContext = pAppContext; }
		IAppContext* getAppContext() { return m_pAppContext; }
	};

	struct PHClass{};

	typedef void (PHClass::*TargetFunc)();

	union TargetProc
	{
		TargetFunc mf_oo;	// TOSTUDY
		void (PHClass::*mf_vv)();
		void (PHClass::*mf_vc)(Jpacket&);
		void (PHClass::*mf_vcc)(Jpacket&, IConn*);
	};

	enum FormProcType
	{
		fpt_vv,
		fpt_vc,
		fpt_vcc,
	};

	struct FormEntry
	{
		uint32_t m_uCmd;
		uint32_t m_uType;
		TargetFunc m_pFunc;
		PHClass* m_pObj;
	};

	#define DECLARE_FORM_MAP \
		static evwork::FormEntry *getFormEntries(); \
		static evwork::FormEntry formEntries[];

	#define BEGIN_FORM_MAP(theClass) \
		FormEntry* theClass::getFormEntries() { return theClass::formEntries; } \
		FormEntry theClass::formEntries[] = {

	#define END_FORM_MAP() \
		{0, fpt_vv, NULL, NULL} \
		};

	#define ON_REQUEST(cmd, fp) \
		{ cmd, fpt_vc, (TargetFunc)(static_cast<void (PHClass::*)(Jpacket&)>(fp)), NULL},

	#define ON_REQUEST_CONN(cmd, fp) \
		{ cmd, fpt_vcc, (TargetFunc)(static_cast<void (PHClass::*)(Jpacket&, IConn*)>(fp)), NULL},

}	 
