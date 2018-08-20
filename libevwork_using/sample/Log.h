#ifndef _LOG_H_
#define _LOG_H_

void loginit();
void logDinit();
void dolog(int level, const char *fmt, ...);

#define DBUG(level, fmt, arg...) \
	do {    \
			dolog(level, fmt, ##arg);\
	} while (0)

#endif
