#ifndef _LINKEV_H_
#define _LINKEV_H_

#include "../EVWork.h"

class LinkEV: public evwork::ILinkEvent
{
	virtual void onConnected(evwork::IConn* pConn);
	virtual void onClose(evwork::IConn* pConn);
};


#endif
